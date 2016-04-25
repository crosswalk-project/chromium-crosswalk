// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef V8HeapProfilerAgentImpl_h
#define V8HeapProfilerAgentImpl_h

#include "platform/v8_inspector/public/V8HeapProfilerAgent.h"
#include "wtf/Forward.h"
#include "wtf/Noncopyable.h"
#include <v8-profiler.h>

namespace blink {

class V8RuntimeAgent;

typedef String ErrorString;

using protocol::Maybe;

class HeapProfileXDK final {
public:
    static PassOwnPtr<HeapProfileXDK> create(v8::HeapEventXDK* event, v8::Isolate* isolate)
    {
        return adoptPtr(new HeapProfileXDK(event, isolate));
    }

    String getSymbols() const;
    String getFrames() const;
    String getTypes() const;
    String getChunks() const;
    String getRetentions() const;
    int getDuration() const;

private:
    HeapProfileXDK(v8::HeapEventXDK* event, v8::Isolate* isolate)
        : m_event(event),
        m_isolate(isolate)
    {
    }

    v8::HeapEventXDK* m_event;
    v8::Isolate* m_isolate;
};

class V8HeapProfilerAgentImpl : public V8HeapProfilerAgent {
    WTF_MAKE_NONCOPYABLE(V8HeapProfilerAgentImpl);
public:
    explicit V8HeapProfilerAgentImpl(v8::Isolate*, V8RuntimeAgent*);
    ~V8HeapProfilerAgentImpl() override;

    void setInspectorState(PassRefPtr<protocol::DictionaryValue> state) override { m_state = state; }
    void setFrontend(protocol::Frontend::HeapProfiler* frontend) override { m_frontend = frontend; }
    void clearFrontend() override;
    void restore() override;

    void collectGarbage(ErrorString*) override;

    void enable(ErrorString*) override;
    void startTrackingHeapObjects(ErrorString*, const Maybe<bool>& trackAllocations) override;
    void stopTrackingHeapObjects(ErrorString*, const Maybe<bool>& reportProgress) override;

    void disable(ErrorString*) override;

    void takeHeapSnapshot(ErrorString*, const Maybe<bool>& reportProgress) override;

    void getObjectByHeapObjectId(ErrorString*, const String& heapSnapshotObjectId, const Maybe<String>& objectGroup, OwnPtr<protocol::Runtime::RemoteObject>* result) override;
    void addInspectedHeapObject(ErrorString*, const String& inspectedHeapObjectId) override;
    void getHeapObjectId(ErrorString*, const String& objectId, String* heapSnapshotObjectId) override;

    void startTrackingHeapXDK(ErrorString*, const Maybe<int>& depth, const Maybe<int>& sav, const Maybe<bool>& retentions) override;
    void stopTrackingHeapXDK(ErrorString*, OwnPtr<protocol::HeapProfiler::HeapEventXDK>*) override;

    void startSampling(ErrorString*) override;
    void stopSampling(ErrorString*, OwnPtr<protocol::HeapProfiler::SamplingHeapProfile>*) override;

    void requestHeapStatsUpdate() override;
    void requestHeapXDKUpdate() override;

private:
    void startTrackingHeapObjectsInternal(bool trackAllocations);
    void stopTrackingHeapObjectsInternal();

    v8::Isolate* m_isolate;
    V8RuntimeAgent* m_runtimeAgent;
    protocol::Frontend::HeapProfiler* m_frontend;
    RefPtr<protocol::DictionaryValue> m_state;
};

} // namespace blink

#endif // !defined(V8HeapProfilerAgentImpl_h)
