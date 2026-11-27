#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>

class ChromaticTuner
{
public:
    static constexpr int bufferSize = 4096; // ~85 ms at 48 kHz -> tracks cleanly down to ~28 Hz

    struct Reading
    {
        bool valid      = false;
        float frequency = 0.0f;
        int midiNote    = 0;
        float cents     = 0.0f;
        juce::String noteName;
    };

    void prepare (double newSampleRate)
    {
        sampleRate = (newSampleRate > 0.0) ? newSampleRate : 44100.0;
        fifo.reset();
        ringBuffer.fill (0.0f);
        analysisBuffer.fill (0.0f);
        yinBuffer.fill (0.0f);
    }

    // Called on the real-time audio thread (lock-free, zero allocations)
    void pushSamples (const float* monoSamples, int numSamples) noexcept
    {
        if (monoSamples == nullptr || numSamples <= 0)
            return;

        // If incoming block exceeds bufferSize, keep only the newest samples
        if (numSamples >= bufferSize)
        {
            monoSamples += (numSamples - bufferSize);
            numSamples = bufferSize;
        }

        const int freeSpace = fifo.getFreeSpace();
        if (freeSpace < numSamples)
            fifo.finishedRead (numSamples - freeSpace);

        const auto scope = fifo.write (numSamples);
        if (scope.blockSize1 > 0)
            std::copy_n (monoSamples, scope.blockSize1, ringBuffer.begin() + scope.startIndex1);
        if (scope.blockSize2 > 0)
            std::copy_n (monoSamples + scope.blockSize1, scope.blockSize2, ringBuffer.begin() + scope.startIndex2);
    }

    // Called on the UI timer thread (~30 Hz) ONLY when the Utility drawer is open
    Reading computePitch()
    {
        const int ready = fifo.getNumReady();
        if (ready > 0)
        {
            // Shift older analysis samples left and append newest samples at the tail
            const int toRead = juce::jmin (ready, bufferSize);
            if (toRead < bufferSize)
                std::move (analysisBuffer.begin() + toRead, analysisBuffer.end(), analysisBuffer.begin());

            const auto scope = fifo.read (toRead);
            int destOffset = bufferSize - toRead;
            if (scope.blockSize1 > 0)
                std::copy_n (ringBuffer.begin() + scope.startIndex1, scope.blockSize1, analysisBuffer.begin() + destOffset);
            if (scope.blockSize2 > 0)
                std::copy_n (ringBuffer.begin() + scope.startIndex2, scope.blockSize2, analysisBuffer.begin() + destOffset + scope.blockSize1);
        }

        // Gate silence below ~ -55 dBFS so room noise doesn't trigger erratic note jumps
        float rms = 0.0f;
        for (float s : analysisBuffer)
            rms += s * s;
        rms = std::sqrt (rms / static_cast<float> (bufferSize));

        if (rms < 0.0018f)
            return {};

        // YIN pitch detection bounds: ~28 Hz (low A0/G#1) up to ~1200 Hz (24th fret high E)
        const int halfSize = bufferSize / 2;
        const int minTau   = juce::jmax (2, static_cast<int> (sampleRate / 1200.0));
        const int maxTau   = juce::jmin (halfSize - 1, static_cast<int> (sampleRate / 28.0));

        if (maxTau <= minTau + 2)
            return {};

        // 1. Difference function d(tau)
        yinBuffer[0] = 1.0f;
        float runningSum = 0.0f;

        for (int tau = 1; tau <= maxTau; ++tau)
        {
            float diffSum = 0.0f;
            for (int i = 0; i < halfSize; ++i)
            {
                const float delta = analysisBuffer[static_cast<size_t> (i)] - analysisBuffer[static_cast<size_t> (i + tau)];
                diffSum += delta * delta;
            }

            // 2. Cumulative Mean Normalized Difference (CMNDF)
            runningSum += diffSum;
            yinBuffer[static_cast<size_t> (tau)] = (runningSum > 0.0f) ? (diffSum * static_cast<float> (tau) / runningSum) : 1.0f;
        }

        // 3. Absolute threshold search (0.15 prevents octave errors on low wound strings)
        constexpr float threshold = 0.15f;
        int tauEstimate = -1;

        for (int tau = minTau; tau < maxTau; ++tau)
        {
            if (yinBuffer[static_cast<size_t> (tau)] < threshold)
            {
                while (tau + 1 < maxTau && yinBuffer[static_cast<size_t> (tau + 1)] < yinBuffer[static_cast<size_t> (tau)])
                    ++tau;
                tauEstimate = tau;
                break;
            }
        }

        // Fallback to global minimum if signal is harmonic-heavy
        if (tauEstimate == -1)
        {
            float minVal = 1.0f;
            for (int tau = minTau; tau < maxTau; ++tau)
            {
                if (yinBuffer[static_cast<size_t> (tau)] < minVal)
                {
                    minVal = yinBuffer[static_cast<size_t> (tau)];
                    tauEstimate = tau;
                }
            }
            if (minVal > 0.35f || tauEstimate <= minTau)
                return {};
        }

        // 4. Parabolic interpolation for sub-sample cent accuracy
        float betterTau = static_cast<float> (tauEstimate);
        if (tauEstimate > minTau && tauEstimate < maxTau - 1)
        {
            const float s0 = yinBuffer[static_cast<size_t> (tauEstimate - 1)];
            const float s1 = yinBuffer[static_cast<size_t> (tauEstimate)];
            const float s2 = yinBuffer[static_cast<size_t> (tauEstimate + 1)];
            const float denom = 2.0f * (2.0f * s1 - s2 - s0);
            if (std::abs (denom) > 1e-6f)
                betterTau += (s2 - s0) / denom;
        }

        const float freq = static_cast<float> (sampleRate) / betterTau;
        if (freq < 27.5f || freq > 1250.0f)
            return {};

        const float midiFloat   = 69.0f + 12.0f * std::log2 (freq / 440.0f);
        const int   nearestMidi = juce::roundToInt (midiFloat);
        const float centsOffset = (midiFloat - static_cast<float> (nearestMidi)) * 100.0f;

        static constexpr const char* noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        const int noteIdx = ((nearestMidi % 12) + 12) % 12;
        const int octave  = (nearestMidi / 12) - 1;

        Reading r;
        r.valid     = true;
        r.frequency = freq;
        r.midiNote  = nearestMidi;
        r.cents     = juce::jlimit (-50.0f, 50.0f, centsOffset);
        r.noteName  = juce::String (noteNames[noteIdx]) + juce::String (octave);
        return r;
    }

private:
    double sampleRate = 44100.0;
    juce::AbstractFifo fifo { bufferSize };
    std::array<float, bufferSize> ringBuffer {};
    std::array<float, bufferSize> analysisBuffer {};
    std::array<float, bufferSize / 2> yinBuffer {};
};