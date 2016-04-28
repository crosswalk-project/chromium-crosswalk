// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/v8_inspector/V8HeapProfilerAgentImpl.h"

#include "platform/v8_inspector/InjectedScript.h"
#include "platform/v8_inspector/InjectedScriptManager.h"
#include "platform/v8_inspector/V8RuntimeAgentImpl.h"
#include "platform/v8_inspector/V8StringUtil.h"
#include "wtf/CurrentTime.h"
#include <v8-profiler.h>

namespace blink {

namespace HeapProfilerAgentState {
static const char heapProfilerEnabled[] = "heapProfilerEnabled";
static const char heapObjectsTrackingEnabled[] = "heapObjectsTrackingEnabled";
static const char allocationTrackingEnabled[] = "allocationTrackingEnabled";
static const char samplingHeapProfilerEnabled[] = "samplingHeapProfilerEnabled";
}

namespace {

class HeapSnapshotProgress final : public v8::ActivityControl {
public:
    HeapSnapshotProgress(protocol::Frontend::HeapProfiler* frontend)
        : m_frontend(frontend) { }
    ControlOption ReportProgressValue(int done, int total) override
    {
        m_frontend->reportHeapSnapshotProgress(done, total, protocol::Maybe<bool>());
        if (done >= total) {
            m_frontend->reportHeapSnapshotProgress(total, total, true);
        }
        m_frontend->flush();
        return kContinue;
    }
private:
    protocol::Frontend::HeapProfiler* m_frontend;
};

class GlobalObjectNameResolver final : public v8::HeapProfiler::ObjectNameResolver {
public:
    explicit GlobalObjectNameResolver(V8RuntimeAgentImpl* runtimeAgent) : m_runtimeAgent(runtimeAgent) { }
    const char* GetName(v8::Local<v8::Object> object) override
    {
        int contextId = V8Debugger::contextId(object->CreationContext());
        if (!contextId)
            return "";
        InjectedScript* injectedScript = m_runtimeAgent->injectedScriptManager()->findInjectedScript(contextId);
        if (!injectedScript)
            return "";
        CString name = injectedScript->origin().utf8();
        m_strings.append(name);
        return name.data();
    }

private:
    Vector<CString> m_strings;
    V8RuntimeAgentImpl* m_runtimeAgent;
};

class HeapSnapshotOutputStream final : public v8::OutputStream {
public:
    HeapSnapshotOutputStream(protocol::Frontend::HeapProfiler* frontend)
        : m_frontend(frontend) { }
    void EndOfStream() override { }
    int GetChunkSize() override { return 102400; }
    WriteResult WriteAsciiChunk(char* data, int size) override
    {
        m_frontend->addHeapSnapshotChunk(String(data, size));
        m_frontend->flush();
        return kContinue;
    }
private:
    protocol::Frontend::HeapProfiler* m_frontend;
};

v8::Local<v8::Object> objectByHeapObjectId(v8::Isolate* isolate, unsigned id)
{
    v8::HeapProfiler* profiler = isolate->GetHeapProfiler();
    v8::Local<v8::Value> value = profiler->FindObjectById(id);
    if (value.IsEmpty() || !value->IsObject())
        return v8::Local<v8::Object>();
    return value.As<v8::Object>();
}

class InspectableHeapObject final : public V8RuntimeAgent::Inspectable {
public:
    explicit InspectableHeapObject(unsigned heapObjectId) : m_heapObjectId(heapObjectId) { }
    v8::Local<v8::Value> get(v8::Local<v8::Context> context) override
    {
        return objectByHeapObjectId(context->GetIsolate(), m_heapObjectId);
    }
private:
    unsigned m_heapObjectId;
};

class HeapStatsStream final : public v8::OutputStream {
public:
    HeapStatsStream(protocol::Frontend::HeapProfiler* frontend)
        : m_frontend(frontend)
    {
    }

    void EndOfStream() override { }

    WriteResult WriteAsciiChunk(char* data, int size) override
    {
        ASSERT(false);
        return kAbort;
    }

    WriteResult WriteHeapStatsChunk(v8::HeapStatsUpdate* updateData, int count) override
    {
        ASSERT(count > 0);
        OwnPtr<protocol::Array<int>> statsDiff = protocol::Array<int>::create();
        for (int i = 0; i < count; ++i) {
            statsDiff->addItem(updateData[i].index);
            statsDiff->addItem(updateData[i].count);
            statsDiff->addItem(updateData[i].size);
        }
        m_frontend->heapStatsUpdate(statsDiff.release());
        return kContinue;
    }

private:
    protocol::Frontend::HeapProfiler* m_frontend;
};

} // namespace

PassOwnPtr<V8HeapProfilerAgent> V8HeapProfilerAgent::create(v8::Isolate* isolate, V8RuntimeAgent* runtimeAgent)
{
    return adoptPtr(new V8HeapProfilerAgentImpl(isolate, runtimeAgent));
}

V8HeapProfilerAgentImpl::V8HeapProfilerAgentImpl(v8::Isolate* isolate, V8RuntimeAgent* runtimeAgent)
    : m_isolate(isolate)
    , m_runtimeAgent(runtimeAgent)
{
}

V8HeapProfilerAgentImpl::~V8HeapProfilerAgentImpl()
{
}

void V8HeapProfilerAgentImpl::clearFrontend()
{
    ErrorString error;
    disable(&error);
    ASSERT(m_frontend);
    m_frontend = nullptr;
}

void V8HeapProfilerAgentImpl::restore()
{
    if (m_state->booleanProperty(HeapProfilerAgentState::heapProfilerEnabled, false))
        m_frontend->resetProfiles();
    if (m_state->booleanProperty(HeapProfilerAgentState::heapObjectsTrackingEnabled, false))
        startTrackingHeapObjectsInternal(m_state->booleanProperty(HeapProfilerAgentState::allocationTrackingEnabled, false));
    if (m_state->booleanProperty(HeapProfilerAgentState::samplingHeapProfilerEnabled, false)) {
        ErrorString error;
        startSampling(&error);
    }
}

void V8HeapProfilerAgentImpl::collectGarbage(ErrorString*)
{
    m_isolate->LowMemoryNotification();
}

void V8HeapProfilerAgentImpl::startTrackingHeapObjects(ErrorString*, const protocol::Maybe<bool>& trackAllocations)
{
    m_state->setBoolean(HeapProfilerAgentState::heapObjectsTrackingEnabled, true);
    bool allocationTrackingEnabled = trackAllocations.fromMaybe(false);
    m_state->setBoolean(HeapProfilerAgentState::allocationTrackingEnabled, allocationTrackingEnabled);
    startTrackingHeapObjectsInternal(allocationTrackingEnabled);
}

void V8HeapProfilerAgentImpl::stopTrackingHeapObjects(ErrorString* error, const protocol::Maybe<bool>& reportProgress)
{
    requestHeapStatsUpdate();
    takeHeapSnapshot(error, reportProgress);
    stopTrackingHeapObjectsInternal();
}

void V8HeapProfilerAgentImpl::enable(ErrorString*)
{
    m_state->setBoolean(HeapProfilerAgentState::heapProfilerEnabled, true);
}

void V8HeapProfilerAgentImpl::disable(ErrorString* error)
{
    stopTrackingHeapObjectsInternal();
    if (m_state->booleanProperty(HeapProfilerAgentState::samplingHeapProfilerEnabled, false)) {
        v8::HeapProfiler* profiler = m_isolate->GetHeapProfiler();
        if (profiler)
            profiler->StopSamplingHeapProfiler();
    }
    m_isolate->GetHeapProfiler()->ClearObjectIds();
    m_state->setBoolean(HeapProfilerAgentState::heapProfilerEnabled, false);
}

void V8HeapProfilerAgentImpl::takeHeapSnapshot(ErrorString* errorString, const protocol::Maybe<bool>& reportProgress)
{
    v8::HeapProfiler* profiler = m_isolate->GetHeapProfiler();
    if (!profiler) {
        *errorString = "Cannot access v8 heap profiler";
        return;
    }
    OwnPtr<HeapSnapshotProgress> progress;
    if (reportProgress.fromMaybe(false))
        progress = adoptPtr(new HeapSnapshotProgress(m_frontend));

    GlobalObjectNameResolver resolver(static_cast<V8RuntimeAgentImpl*>(m_runtimeAgent));
    const v8::HeapSnapshot* snapshot = profiler->TakeHeapSnapshot(progress.get(), &resolver);
    if (!snapshot) {
        *errorString = "Failed to take heap snapshot";
        return;
    }
    HeapSnapshotOutputStream stream(m_frontend);
    snapshot->Serialize(&stream);
    const_cast<v8::HeapSnapshot*>(snapshot)->Delete();
}

void V8HeapProfilerAgentImpl::getObjectByHeapObjectId(ErrorString* error, const String& heapSnapshotObjectId, const protocol::Maybe<String>& objectGroup, OwnPtr<protocol::Runtime::RemoteObject>* result)
{
    bool ok;
    unsigned id = heapSnapshotObjectId.toUInt(&ok);
    if (!ok) {
        *error = "Invalid heap snapshot object id";
        return;
    }

    v8::HandleScope handles(m_isolate);
    v8::Local<v8::Object> heapObject = objectByHeapObjectId(m_isolate, id);
    if (heapObject.IsEmpty()) {
        *error = "Object is not available";
        return;
    }
    *result = m_runtimeAgent->wrapObject(heapObject->CreationContext(), heapObject, objectGroup.fromMaybe(""));
    if (!result)
        *error = "Object is not available";
}

void V8HeapProfilerAgentImpl::addInspectedHeapObject(ErrorString* errorString, const String& inspectedHeapObjectId)
{
    bool ok;
    unsigned id = inspectedHeapObjectId.toUInt(&ok);
    if (!ok) {
        *errorString = "Invalid heap snapshot object id";
        return;
    }
    m_runtimeAgent->addInspectedObject(adoptPtr(new InspectableHeapObject(id)));
}

void V8HeapProfilerAgentImpl::getHeapObjectId(ErrorString* errorString, const String& objectId, String* heapSnapshotObjectId)
{
    v8::HandleScope handles(m_isolate);
    v8::Local<v8::Value> value = m_runtimeAgent->findObject(objectId);
    if (value.IsEmpty() || value->IsUndefined()) {
        *errorString = "Object with given id not found";
        return;
    }

    v8::SnapshotObjectId id = m_isolate->GetHeapProfiler()->GetObjectId(value);
    *heapSnapshotObjectId = String::number(id);
}

void V8HeapProfilerAgentImpl::requestHeapStatsUpdate()
{
    if (!m_frontend)
        return;
    HeapStatsStream stream(m_frontend);
    v8::SnapshotObjectId lastSeenObjectId = m_isolate->GetHeapProfiler()->GetHeapStats(&stream);
    m_frontend->lastSeenObjectId(lastSeenObjectId, WTF::currentTimeMS());
}

void V8HeapProfilerAgentImpl::startTrackingHeapObjectsInternal(bool trackAllocations)
{
    m_isolate->GetHeapProfiler()->StartTrackingHeapObjects(trackAllocations);
}

void V8HeapProfilerAgentImpl::stopTrackingHeapObjectsInternal()
{
    m_isolate->GetHeapProfiler()->StopTrackingHeapObjects();
    m_state->setBoolean(HeapProfilerAgentState::heapObjectsTrackingEnabled, false);
    m_state->setBoolean(HeapProfilerAgentState::allocationTrackingEnabled, false);
}

void V8HeapProfilerAgentImpl::startSampling(ErrorString* errorString)
{
    v8::HeapProfiler* profiler = m_isolate->GetHeapProfiler();
    if (!profiler) {
        *errorString = "Cannot access v8 heap profiler";
        return;
    }
    m_state->setBoolean(HeapProfilerAgentState::samplingHeapProfilerEnabled, true);
    profiler->StartSamplingHeapProfiler();
}

namespace {
PassOwnPtr<protocol::HeapProfiler::SamplingHeapProfileNode> buildSampingHeapProfileNode(const v8::AllocationProfile::Node* node)
{
    auto children = protocol::Array<protocol::HeapProfiler::SamplingHeapProfileNode>::create();
    for (const auto* child : node->children)
        children->addItem(buildSampingHeapProfileNode(child));
    size_t totalSize = 0;
    for (const auto& allocation : node->allocations)
        totalSize += allocation.size * allocation.count;
    OwnPtr<protocol::HeapProfiler::SamplingHeapProfileNode> result = protocol::HeapProfiler::SamplingHeapProfileNode::create()
        .setFunctionName(toWTFString(node->name))
        .setScriptId(String::number(node->script_id))
        .setUrl(toWTFString(node->script_name))
        .setLineNumber(node->line_number)
        .setColumnNumber(node->column_number)
        .setTotalSize(totalSize)
        .setChildren(children).build();
    return result.release();
}
} // namespace

void V8HeapProfilerAgentImpl::stopSampling(ErrorString* errorString, OwnPtr<protocol::HeapProfiler::SamplingHeapProfile>* profile)
{
    v8::HeapProfiler* profiler = m_isolate->GetHeapProfiler();
    if (!profiler) {
        *errorString = "Cannot access v8 heap profiler";
        return;
    }
    v8::HandleScope scope(m_isolate); // Allocation profile contains Local handles.
    OwnPtr<v8::AllocationProfile> v8Profile = adoptPtr(profiler->GetAllocationProfile());
    profiler->StopSamplingHeapProfiler();
    m_state->setBoolean(HeapProfilerAgentState::samplingHeapProfilerEnabled, false);
    if (!v8Profile) {
        *errorString = "Cannot access v8 sampled heap profile.";
        return;
    }
    v8::AllocationProfile::Node* root = v8Profile->GetRootNode();
    *profile = protocol::HeapProfiler::SamplingHeapProfile::create()
        .setHead(buildSampingHeapProfileNode(root)).build();
}

class HeapXDKStream : public v8::OutputStream {
public:
    HeapXDKStream(protocol::Frontend::HeapProfiler* frontend)
        : m_frontend(frontend)
    {
    }
    void EndOfStream() override { }

    WriteResult WriteAsciiChunk(char* data, int size) override
    {
        ASSERT(false);
        return kAbort;
    }

    WriteResult WriteHeapXDKChunk(const char* symbols, size_t symbolsSize, const char* frames, size_t framesSize, const char* types, size_t typesSize,
        const char* chunks, size_t chunksSize, const char* retentions, size_t retentionSize) override
    {
        m_frontend->heapXDKUpdate(String(symbols, symbolsSize), String(frames, framesSize), String(types, typesSize), String(chunks, chunksSize), String(retentions, retentionSize));
        return kContinue;
    }

private:
    protocol::Frontend::HeapProfiler* m_frontend;
};

static PassOwnPtr<protocol::HeapProfiler::HeapEventXDK> createHeapProfileXDK(const HeapProfileXDK& heapProfileXDK)
{
    OwnPtr<protocol::HeapProfiler::HeapEventXDK> profile = protocol::HeapProfiler::HeapEventXDK::create()
        .setDuration(heapProfileXDK.getDuration())
        .setSymbols(heapProfileXDK.getSymbols())
        .setFrames(heapProfileXDK.getFrames())
        .setTypes(heapProfileXDK.getTypes())
        .setChunks(heapProfileXDK.getChunks())
        .setRetentions(heapProfileXDK.getRetentions()).build();
    return profile.release();
}

void V8HeapProfilerAgentImpl::startTrackingHeapXDK(ErrorString* error, const protocol::Maybe<int>& depth, const protocol::Maybe<int>& sav, const protocol::Maybe<bool>& retentions)
{
    v8::HeapProfiler* profiler = m_isolate->GetHeapProfiler();
    if (!profiler) {
        *error = "Cannot access v8 heap profiler";
        return;
    }

    m_state->setBoolean(HeapProfilerAgentState::heapObjectsTrackingEnabled, true);
    int stackDepth = depth.fromMaybe(8);
    bool needRetentions = retentions.fromMaybe(false);
    profiler->StartTrackingHeapObjectsXDK(stackDepth, needRetentions);
}

void V8HeapProfilerAgentImpl::stopTrackingHeapXDK(ErrorString* error, OwnPtr<protocol::HeapProfiler::HeapEventXDK>* profile)
{
    v8::HeapProfiler* profiler = m_isolate->GetHeapProfiler();
    if (!profiler) {
        *error = "Cannot access v8 heap profiler";
        return;
    }

    OwnPtr<HeapProfileXDK> heapProfileXDK = HeapProfileXDK::create(
        profiler->StopTrackingHeapObjectsXDK(), m_isolate);
    *profile = createHeapProfileXDK(*heapProfileXDK);
    m_state->setBoolean(HeapProfilerAgentState::heapObjectsTrackingEnabled, false);
}

void V8HeapProfilerAgentImpl::requestHeapXDKUpdate()
{
    if (!m_frontend)
        return;
    HeapXDKStream heapXDKStream(m_frontend);
    m_isolate->GetHeapProfiler()->GetHeapXDKStats(
        &heapXDKStream);
}

String HeapProfileXDK::getSymbols() const
{
    v8::HandleScope handleScope(m_isolate);
    return String(m_event->getSymbols());
}

String HeapProfileXDK::getFrames() const
{
    v8::HandleScope handleScope(m_isolate);
    return String(m_event->getFrames());
}

String HeapProfileXDK::getTypes() const
{
    v8::HandleScope handleScope(m_isolate);
    return String(m_event->getTypes());
}

String HeapProfileXDK::getChunks() const
{
    v8::HandleScope handleScope(m_isolate);
    return String(m_event->getChunks());
}

int HeapProfileXDK::getDuration() const
{
    return (int)m_event->getDuration();
}

String HeapProfileXDK::getRetentions() const
{
    v8::HandleScope handleScope(m_isolate);
    return String(m_event->getRetentions());
}

} // namespace blink
