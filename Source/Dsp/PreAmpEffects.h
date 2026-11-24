#pragma once
#include "DspHelpers.h"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>

namespace FollowerDsp
{
    //==========================================================================
    // 1. Analog VCA Tremolo (Pre-Amp)
    //==========================================================================
    class Tremolo
    {
    public:
        void prepare (double sr) noexcept
        {
            sampleRate = sr;
            phase = 0.0;
        }

        void reset() noexcept { phase = 0.0; }

        void setParameters (float rateHz, float depth0to1, float wave0to1) noexcept
        {
            freq = std::clamp (rateHz, 0.4f, 14.0f);
            depth = std::clamp (depth0to1, 0.0f, 1.0f);
            waveShape = std::clamp (wave0to1, 0.0f, 1.0f); // 0 = Triangle, 1 = Choppy Square
        }

        float processSample (float in) noexcept
        {
            phase += static_cast<double> (freq) / sampleRate;
            if (phase >= 1.0) phase -= 1.0;

            // Triangle LFO [0..1]
            const float tri = 1.0f - 2.0f * std::abs (static_cast<float> (phase) - 0.5f);
            // Morph smoothly from Triangle toward Rounded Square (TR-2 Wave knob)
            const float sharp = 1.0f + waveShape * 11.0f;
            const float shaped = 0.5f + 0.5f * std::tanh ((tri - 0.5f) * 2.0f * sharp) / std::tanh (sharp);

            // TR-2 VCA gain modulation with slight makeup gain (+1.5 dB at full depth)
            const float gainMod = (1.0f - depth * (1.0f - shaped)) * (1.0f + depth * 0.18f);
            return in * gainMod;
        }

    private:
        double sampleRate = 48000.0;
        double phase = 0.0;
        float freq = 3.5f;
        float depth = 0.5f;
        float waveShape = 0.25f;
    };

    //==========================================================================
    // 2. BBD Analog Delay (Pre-Amp)
    //==========================================================================
    class AnalogDelay
    {
    public:
        void prepare (double sr) noexcept
        {
            sampleRate = sr;
            const size_t maxSamples = static_cast<size_t> (sr * 0.65) + 8; // Up to 600ms
            buffer.assign (maxSamples, 0.0f);
            writeIdx = 0;
            bbdFilter1.setLowPass (sr, 3100.0f);
            bbdFilter2.setLowPass (sr, 3600.0f);
            fbHpf.setHighPass (sr, 110.0f);
            smoothedDelaySamples = static_cast<float> (sr * 0.22);
        }

        void reset() noexcept
        {
            std::fill (buffer.begin(), buffer.end(), 0.0f);
            writeIdx = 0;
            bbdFilter1.reset();
            bbdFilter2.reset();
            fbHpf.reset();
        }

        void setParameters (float timeMs, float feedback0to1, float mix0to1) noexcept
        {
            targetDelaySamples = std::clamp (timeMs, 20.0f, 550.0f) * 0.001f * static_cast<float> (sampleRate);
            // DM-2 allows warm runaway self-oscillation above ~0.88
            feedback = std::clamp (feedback0to1, 0.0f, 1.08f);
            mix = std::clamp (mix0to1, 0.0f, 1.0f);
        }

        float processSample (float in) noexcept
        {
            if (buffer.empty()) return in;

            // Smooth delay time slewing for analog pitch-warp when turning Repeat Rate
            smoothedDelaySamples += 0.0015f * (targetDelaySamples - smoothedDelaySamples);

            const size_t bufSize = buffer.size();
            float readPos = static_cast<float> (writeIdx) - smoothedDelaySamples;
            while (readPos < 0.0f) readPos += static_cast<float> (bufSize);

            const size_t idx0 = static_cast<size_t> (readPos) % bufSize;
            const size_t idx1 = (idx0 + 1) % bufSize;
            const float frac = readPos - std::floor (readPos);
            const float delayed = buffer[idx0] * (1.0f - frac) + buffer[idx1] * frac;

            // BBD clock anti-alias/reconstruction filtering + compander soft saturation
            float filtered = bbdFilter2.process (bbdFilter1.process (delayed));
            filtered = fbHpf.process (filtered);
            const float fbSignal = std::tanh ((in + filtered * feedback) * 0.85f) * 1.15f;

            buffer[writeIdx] = fbSignal;
            writeIdx = (writeIdx + 1) % bufSize;

            return in + filtered * mix;
        }

    private:
        double sampleRate = 48000.0;
        std::vector<float> buffer;
        size_t writeIdx = 0;
        float targetDelaySamples = 10000.0f;
        float smoothedDelaySamples = 10000.0f;
        float feedback = 0.35f;
        float mix = 0.35f;
        OnePoleFilter bbdFilter1, bbdFilter2, fbHpf;
    };

    //==========================================================================
    // 3. Algorithmic Hall Reverb (Pre-Amp Cavernous Post-Metal Wash)
    //==========================================================================
    class PreAmpHallReverb
    {
    public:
        void prepare (double sr) noexcept
        {
            sampleRate = sr;
            const double scale = sr / 48000.0;

            // Prime-spaced delay lengths for a lush, dense 8-channel Hadamard FDN Hall
            constexpr int baseLengths[8] = { 1693, 1949, 2237, 2551, 2879, 3221, 3581, 3967 };
            constexpr int apLengths[4]   = { 241, 337, 461, 593 };

            for (int i = 0; i < 8; ++i)
            {
                const size_t len = static_cast<size_t> (baseLengths[i] * scale) + 4;
                fdnLines[i].assign (len, 0.0f);
                fdnIdx[i] = 0;
                dampFilters[i].setLowPass (sr, 4200.0f);
            }

            for (int i = 0; i < 4; ++i)
            {
                const size_t len = static_cast<size_t> (apLengths[i] * scale) + 4;
                diffusers[i].assign (len, 0.0f);
                diffIdx[i] = 0;
            }

            preDelayBuf.assign (static_cast<size_t> (sr * 0.16) + 4, 0.0f);
            preDelayIdx = 0;
            inputHpf.setHighPass (sr, 120.0f);
            lfoPhase = 0.0;
        }

        void reset() noexcept
        {
            for (int i = 0; i < 8; ++i)
            {
                std::fill (fdnLines[i].begin(), fdnLines[i].end(), 0.0f);
                fdnIdx[i] = 0;
                dampFilters[i].reset();
            }
            for (int i = 0; i < 4; ++i)
            {
                std::fill (diffusers[i].begin(), diffusers[i].end(), 0.0f);
                diffIdx[i] = 0;
            }
            std::fill (preDelayBuf.begin(), preDelayBuf.end(), 0.0f);
            preDelayIdx = 0;
            inputHpf.reset();
        }

        void setParameters (float decaySec, float dampingHz, float preDelayMs, float mix0to1) noexcept
        {
            const float dec = std::clamp (decaySec, 0.4f, 12.0f);
            // Average delay ~2750 samples @ 48kHz (~57ms)
            const float rt60Blocks = (dec * static_cast<float> (sampleRate)) / 2750.0f;
            feedbackGain = std::clamp (std::pow (10.0f, -3.0f / std::max (1.0f, rt60Blocks)), 0.45f, 0.985f);

            const float damp = std::clamp (dampingHz, 900.0f, 11000.0f);
            for (int i = 0; i < 8; ++i)
                dampFilters[i].setLowPass (sampleRate, damp);

            preDelaySamples = std::clamp (preDelayMs, 0.0f, 140.0f) * 0.001f * static_cast<float> (sampleRate);
            mix = std::clamp (mix0to1, 0.0f, 1.0f);
        }

        float processSample (float in) noexcept
        {
            if (preDelayBuf.empty()) return in;

            // Pre-delay line
            const size_t pdSize = preDelayBuf.size();
            const size_t pdOffset = std::min (pdSize - 1, static_cast<size_t> (preDelaySamples));
            const size_t pdRead = (preDelayIdx + pdSize - pdOffset) % pdSize;
            const float preDelayed = preDelayBuf[pdRead];
            preDelayBuf[preDelayIdx] = inputHpf.process (in);
            preDelayIdx = (preDelayIdx + 1) % pdSize;

            // 4-stage series allpass input diffuser for smooth hall wash
            float diff = preDelayed;
            for (int i = 0; i < 4; ++i)
            {
                const float bufOut = diffusers[i][diffIdx[i]];
                const float v = diff + 0.58f * bufOut;
                diffusers[i][diffIdx[i]] = v;
                diff = bufOut - 0.58f * v;
                diffIdx[i] = (diffIdx[i] + 1) % diffusers[i].size();
            }

            // Gentle chorus modulation inside the tank to prevent metallic ringing
            lfoPhase += 0.65 / sampleRate;
            if (lfoPhase >= 1.0) lfoPhase -= 1.0;
            const float mod = std::sin (2.0f * 3.14159265359f * static_cast<float> (lfoPhase)) * 3.5f;

            float tap[8];
            for (int i = 0; i < 8; ++i)
            {
                const size_t len = fdnLines[i].size();
                const float offset = (i % 2 == 0) ? mod : -mod;
                float rPos = static_cast<float> (fdnIdx[i]) + offset;
                while (rPos < 0.0f) rPos += static_cast<float> (len);
                const size_t i0 = static_cast<size_t> (rPos) % len;
                const size_t i1 = (i0 + 1) % len;
                const float f = rPos - std::floor (rPos);
                const float rawTap = fdnLines[i][i0] * (1.0f - f) + fdnLines[i][i1] * f;
                tap[i] = dampFilters[i].process (rawTap) * feedbackGain;
            }

            // Fast Walsh-Hadamard 8x8 lossless mixing matrix
            float s0 = tap[0] + tap[1], d0 = tap[0] - tap[1];
            float s1 = tap[2] + tap[3], d1 = tap[2] - tap[3];
            float s2 = tap[4] + tap[5], d2 = tap[4] - tap[5];
            float s3 = tap[6] + tap[7], d3 = tap[6] - tap[7];

            float ss0 = s0 + s1, sd0 = s0 - s1;
            float dd0 = d0 + d1, dd1 = d0 - d1;
            float ss1 = s2 + s3, sd1 = s2 - s3;
            float dd2 = d2 + d3, dd3 = d2 - d3;

            constexpr float norm = 0.35355339f; // 1 / sqrt(8)
            const float mixed[8] = {
                (ss0 + ss1) * norm, (dd0 + dd2) * norm,
                (sd0 + sd1) * norm, (dd1 + dd3) * norm,
                (ss0 - ss1) * norm, (dd0 - dd2) * norm,
                (sd0 - sd1) * norm, (dd1 - dd3) * norm
            };

            float wetSum = 0.0f;
            for (int i = 0; i < 8; ++i)
            {
                const float inject = diff * ((i % 2 == 0) ? 0.35f : -0.35f);
                fdnLines[i][fdnIdx[i]] = mixed[i] + inject;
                fdnIdx[i] = (fdnIdx[i] + 1) % fdnLines[i].size();
                wetSum += tap[i];
            }

            return in + (wetSum * 0.42f) * mix;
        }

    private:
        double sampleRate = 48000.0;
        std::array<std::vector<float>, 8> fdnLines;
        std::array<size_t, 8> fdnIdx {};
        std::array<OnePoleFilter, 8> dampFilters;

        std::array<std::vector<float>, 4> diffusers;
        std::array<size_t, 4> diffIdx {};

        std::vector<float> preDelayBuf;
        size_t preDelayIdx = 0;
        OnePoleFilter inputHpf;

        double lfoPhase = 0.0;
        float feedbackGain = 0.85f;
        float preDelaySamples = 1200.0f;
        float mix = 0.35f;
    };
}
