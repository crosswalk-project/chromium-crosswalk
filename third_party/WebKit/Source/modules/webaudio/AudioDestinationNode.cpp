/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "modules/webaudio/AudioDestinationNode.h"
#include "modules/webaudio/AbstractAudioContext.h"
#include "modules/webaudio/AudioNodeInput.h"
#include "modules/webaudio/AudioNodeOutput.h"
#include "platform/audio/AudioUtilities.h"
#include "platform/audio/DenormalDisabler.h"
#include "wtf/Atomics.h"

namespace blink {

AudioDestinationHandler::AudioDestinationHandler(AudioNode& node, float sampleRate)
    : AudioHandler(NodeTypeDestination, node, sampleRate)
    , m_currentSampleFrame(0)
{
    addInput();
}

AudioDestinationHandler::~AudioDestinationHandler()
{
    ASSERT(!isInitialized());
}

void AudioDestinationHandler::render(AudioBus* sourceBus, AudioBus* destinationBus, size_t numberOfFrames)
{
    // We don't want denormals slowing down any of the audio processing
    // since they can very seriously hurt performance.
    // This will take care of all AudioNodes because they all process within this scope.
    DenormalDisabler denormalDisabler;

    // Need to check if the context actually alive. Otherwise the subsequent
    // steps will fail. If the context is not alive somehow, return immediately
    // and do nothing.
    //
    // TODO(hongchan): because the context can go away while rendering, so this
    // check cannot guarantee the safe execution of the following steps.
    ASSERT(context());
    if (!context())
        return;

    context()->deferredTaskHandler().setAudioThreadToCurrentThread();

    // If the destination node is not initialized, pass the silence to the final
    // audio destination (one step before the FIFO). This check is for the case
    // where the destination is in the middle of tearing down process.
    if (!isInitialized()) {
        destinationBus->zero();
        return;
    }

    // Let the context take care of any business at the start of each render quantum.
    context()->handlePreRenderTasks();

    // Prepare the local audio input provider for this render quantum.
    if (sourceBus)
        m_localAudioInputProvider.set(sourceBus);

    ASSERT(numberOfInputs() >= 1);
    if (numberOfInputs() < 1) {
        destinationBus->zero();
        return;
    }
    // This will cause the node(s) connected to us to process, which in turn will pull on their input(s),
    // all the way backwards through the rendering graph.
    AudioBus* renderedBus = input(0).pull(destinationBus, numberOfFrames);

    if (!renderedBus) {
        destinationBus->zero();
    } else if (renderedBus != destinationBus) {
        // in-place processing was not possible - so copy
        destinationBus->copyFrom(*renderedBus);
    }

    // Process nodes which need a little extra help because they are not connected to anything, but still need to process.
    context()->deferredTaskHandler().processAutomaticPullNodes(numberOfFrames);

    // Let the context take care of any business at the end of each render quantum.
    context()->handlePostRenderTasks();

    // Advance current sample-frame.
    size_t newSampleFrame = m_currentSampleFrame + numberOfFrames;
    releaseStore(&m_currentSampleFrame, newSampleFrame);
}

// ----------------------------------------------------------------

AudioDestinationNode::AudioDestinationNode(AbstractAudioContext& context)
    : AudioNode(context)
{
}

AudioDestinationHandler& AudioDestinationNode::audioDestinationHandler() const
{
    return static_cast<AudioDestinationHandler&>(handler());
}

unsigned long AudioDestinationNode::maxChannelCount() const
{
    return audioDestinationHandler().maxChannelCount();
}

} // namespace blink

