/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "platform/audio/AudioDestination.h"

#include "platform/Histogram.h"
#include "platform/audio/AudioFIFO.h"
#include "platform/audio/AudioPullFIFO.h"
#include "public/platform/Platform.h"
#include "public/platform/WebSecurityOrigin.h"

namespace blink {

// Buffer size at which the web audio engine will render.
const unsigned renderBufferSize = 128;

// Size of the FIFO
const size_t fifoSize = 8192;

// Factory method: Chromium-implementation
PassOwnPtr<AudioDestination> AudioDestination::create(AudioIOCallback& callback, const String& inputDeviceId, unsigned numberOfInputChannels, unsigned numberOfOutputChannels, float sampleRate, const PassRefPtr<SecurityOrigin>& securityOrigin)
{
    return adoptPtr(new AudioDestination(callback, inputDeviceId, numberOfInputChannels, numberOfOutputChannels, sampleRate, securityOrigin));
}

AudioDestination::AudioDestination(AudioIOCallback& callback, const String& inputDeviceId, unsigned numberOfInputChannels, unsigned numberOfOutputChannels, float sampleRate, const PassRefPtr<SecurityOrigin>& securityOrigin)
    : m_callback(callback)
    , m_numberOfOutputChannels(numberOfOutputChannels)
    , m_inputBus(AudioBus::create(numberOfInputChannels, renderBufferSize))
    , m_renderBus(AudioBus::create(numberOfOutputChannels, renderBufferSize, false))
    , m_sampleRate(sampleRate)
    , m_isPlaying(false)
{
    // Histogram for audioHardwareBufferSize
    DEFINE_STATIC_LOCAL(SparseHistogram, hardwareBufferSizeHistogram,
        ("WebAudio.AudioDestination.HardwareBufferSize"));
    // Histogram for the actual callback size used.  Typically, this is the same as
    // audioHardwareBufferSize, but can be adjusted depending on some heuristics below.
    DEFINE_STATIC_LOCAL(SparseHistogram, callbackBufferSizeHistogram,
        ("WebAudio.AudioDestination.CallbackBufferSize"));

    // Use the optimal buffer size recommended by the audio backend.
    size_t recommendedHardwareBufferSize = Platform::current()->audioHardwareBufferSize();
    m_callbackBufferSize = recommendedHardwareBufferSize;

#if OS(ANDROID)
    // The optimum low-latency hardware buffer size is usually too small on Android for WebAudio to
    // render without glitching. So, if it is small, use a larger size. If it was already large, use
    // the requested size.
    //
    // Since WebAudio renders in 128-frame blocks, the small buffer sizes (144 for a Galaxy Nexus),
    // cause significant processing jitter. Sometimes multiple blocks will processed, but other
    // times will not be since the FIFO can satisfy the request. By using a larger
    // callbackBufferSize, we smooth out the jitter.
    const size_t kSmallBufferSize = 1024;
    const size_t kDefaultCallbackBufferSize = 2048;

    if (m_callbackBufferSize <= kSmallBufferSize)
        m_callbackBufferSize = kDefaultCallbackBufferSize;
#endif

    // Quick exit if the requested size is too large.
    ASSERT(m_callbackBufferSize + renderBufferSize <= fifoSize);
    if (m_callbackBufferSize + renderBufferSize > fifoSize)
        return;

    m_audioDevice = adoptPtr(Platform::current()->createAudioDevice(m_callbackBufferSize, numberOfInputChannels, numberOfOutputChannels, sampleRate, this, inputDeviceId, securityOrigin));
    ASSERT(m_audioDevice);

    // Record the sizes if we successfully created an output device.
    hardwareBufferSizeHistogram.sample(recommendedHardwareBufferSize);
    callbackBufferSizeHistogram.sample(m_callbackBufferSize);

    // Create a FIFO to handle the possibility of the callback size
    // not being a multiple of the render size. If the FIFO already
    // contains enough data, the data will be provided directly.
    // Otherwise, the FIFO will call the provider enough times to
    // satisfy the request for data.
    m_fifo = adoptPtr(new AudioPullFIFO(*this, numberOfOutputChannels, fifoSize, renderBufferSize));

    // Input buffering.
    m_inputFifo = adoptPtr(new AudioFIFO(numberOfInputChannels, fifoSize));

    // If the callback size does not match the render size, then we need to buffer some
    // extra silence for the input. Otherwise, we can over-consume the input FIFO.
    if (m_callbackBufferSize != renderBufferSize) {
        // FIXME: handle multi-channel input and don't hard-code to stereo.
        RefPtr<AudioBus> silence = AudioBus::create(2, renderBufferSize);
        m_inputFifo->push(silence.get());
    }
}

AudioDestination::~AudioDestination()
{
    stop();
}

void AudioDestination::start()
{
    if (!m_isPlaying && m_audioDevice) {
        m_audioDevice->start();
        m_isPlaying = true;
    }
}

void AudioDestination::stop()
{
    if (m_isPlaying && m_audioDevice) {
        m_audioDevice->stop();
        m_isPlaying = false;
    }
}

float AudioDestination::hardwareSampleRate()
{
    return static_cast<float>(Platform::current()->audioHardwareSampleRate());
}

unsigned long AudioDestination::maxChannelCount()
{
    return static_cast<float>(Platform::current()->audioHardwareOutputChannels());
}

void AudioDestination::render(const WebVector<float*>& sourceData, const WebVector<float*>& audioData, size_t numberOfFrames)
{
    bool isNumberOfChannelsGood = audioData.size() == m_numberOfOutputChannels;
    if (!isNumberOfChannelsGood) {
        ASSERT_NOT_REACHED();
        return;
    }

    bool isBufferSizeGood = numberOfFrames == m_callbackBufferSize;
    if (!isBufferSizeGood) {
        ASSERT_NOT_REACHED();
        return;
    }

    // Buffer optional live input.
    if (sourceData.size() >= 2) {
        // FIXME: handle multi-channel input and don't hard-code to stereo.
        RefPtr<AudioBus> wrapperBus = AudioBus::create(2, numberOfFrames, false);
        wrapperBus->setChannelMemory(0, sourceData[0], numberOfFrames);
        wrapperBus->setChannelMemory(1, sourceData[1], numberOfFrames);
        m_inputFifo->push(wrapperBus.get());
    }

    for (unsigned i = 0; i < m_numberOfOutputChannels; ++i)
        m_renderBus->setChannelMemory(i, audioData[i], numberOfFrames);

    m_fifo->consume(m_renderBus.get(), numberOfFrames);
}

void AudioDestination::provideInput(AudioBus* bus, size_t framesToProcess)
{
    AudioBus* sourceBus = nullptr;
    if (m_inputFifo->framesInFifo() >= framesToProcess) {
        m_inputFifo->consume(m_inputBus.get(), framesToProcess);
        sourceBus = m_inputBus.get();
    }

    m_callback.render(sourceBus, bus, framesToProcess);
}

} // namespace blink

