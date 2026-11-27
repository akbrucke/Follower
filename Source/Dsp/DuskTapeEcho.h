#pragma once
#include "DspHelpers.h"
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <random>

namespace FollowerDsp
{
    /**
     * Post-Cabinet True Stereo 3-Head Tape Echo & Spring Reverb Engine
     * Inspired by Dusk Audio Tape Echo (GPL-3.0) architecture:
     * - Independent Left & Right tape loops with subtle stereo head spacing & quadrature wow/flutter
     * - 3 playback heads per channel (Head 1: ~69-177ms, Head 2: ~131-337ms, Head 3: ~189-489ms)
     * - 12 authentic selector modes (Heads 1..3, multi-head combos, + Spring Reverb, Reverb Only)
     * - Capstan motor inertia, tape age (New/Used/Old), magnetic tape hysteresis saturation,
     *   and Bass/Treble echo-return EQ
     */
    class DuskTapeEcho
    {
    public:
        void prepare (double sr) noexcept
        {
            sampleRate = sr;
            const size_t maxBuf = static_cast<size_t> (sr * 1.2) + 16;

            for (int ch = 0; ch < 2; ++ch)
            {
                auto& c = channels[static_cast<size_t> (ch)];
                c.tapeBuffer.assign (maxBuf, 0.0f);
                c.writePos = 0;

                c.recPreEmphasis.setHighShelf (sr, 2200.0f, 3.5f);
                c.playDeEmphasis.setHighShelf (sr, 2200.0f, -3.5f);
                c.tapeAgeLpf.setLowPass (sr, 6500.0f);
                c.fbHpf.setHighPass (sr, 95.0f);
                c.echoBass.setLowShelf (sr, 180.0f, 0.0f);
                c.echoTreble.setHighShelf (sr, 2800.0f, 0.0f);

                // Slightly staggered 3-spring reverb tank lengths between L and R for natural stereo width
                constexpr int springLens[2][3] = {
                    { 1361, 1789, 2213 },
                    { 1423, 1847, 2281 }
                };
                for (int i = 0; i < 3; ++i)
                {
                    c.springLines[i].assign (static_cast<size_t> (springLens[ch][i] * (sr / 48000.0)) + 4, 0.0f);
                    c.springIdx[i] = 0;
                    c.springDamp[i].setLowPass (sr, 3200.0f);
                }
            }

            smoothedRateNorm = 0.5f;
            wowPhase = 0.0;
            flutterPhase = 0.0;
        }

        void reset() noexcept
        {
            for (int ch = 0; ch < 2; ++ch)
            {
                auto& c = channels[static_cast<size_t> (ch)];
                std::fill (c.tapeBuffer.begin(), c.tapeBuffer.end(), 0.0f);
                c.writePos = 0;
                c.recPreEmphasis.reset();
                c.playDeEmphasis.reset();
                c.tapeAgeLpf.reset();
                c.fbHpf.reset();
                c.echoBass.reset();
                c.echoTreble.reset();
                for (int i = 0; i < 3; ++i)
                {
                    std::fill (c.springLines[i].begin(), c.springLines[i].end(), 0.0f);
                    c.springIdx[i] = 0;
                    c.springDamp[i].reset();
                }
            }
        }

        void setParameters (int mode0to11, float rate0to1, float intensity0to1,
                            float echoLvl0to1, float reverbLvl0to1,
                            float bassDb, float trebleDb,
                            int tapeAge0to2, float wowFlutter0to1, float mix0to1) noexcept
        {
            mode = std::clamp (mode0to11, 0, 11);
            targetRateNorm = std::clamp (rate0to1, 0.0f, 1.0f);
            intensity = std::clamp (intensity0to1, 0.0f, 1.12f);
            echoLevel = std::clamp (echoLvl0to1, 0.0f, 1.5f);
            reverbLevel = std::clamp (reverbLvl0to1, 0.0f, 1.2f);
            tapeAge = std::clamp (tapeAge0to2, 0, 2);
            wowFlutter = std::clamp (wowFlutter0to1, 0.0f, 1.0f);
            mix = std::clamp (mix0to1, 0.0f, 1.0f);

            const float ageCutoff = (tapeAge == 0) ? 8800.0f : (tapeAge == 1 ? 5400.0f : 3400.0f);
            for (int ch = 0; ch < 2; ++ch)
            {
                auto& c = channels[static_cast<size_t> (ch)];
                c.echoBass.setLowShelf (sampleRate, 180.0f, std::clamp (bassDb, -12.0f, 12.0f));
                c.echoTreble.setHighShelf (sampleRate, 2800.0f, std::clamp (trebleDb, -12.0f, 12.0f));
                c.tapeAgeLpf.setLowPass (sampleRate, ageCutoff);
            }
        }

        void processStereoSample (float& left, float& right) noexcept
        {
            if (channels[0].tapeBuffer.empty())
                return;

            // Shared capstan motor inertia smoothing
            smoothedRateNorm += 0.0008f * (targetRateNorm - smoothedRateNorm);

            // Shared wow (~1.42 Hz) & flutter (~6.85 Hz) oscillators with 90-deg stereo phase offset
            wowPhase += 1.42 / sampleRate;
            if (wowPhase >= 1.0) wowPhase -= 1.0;
            flutterPhase += 6.85 / sampleRate;
            if (flutterPhase >= 1.0) flutterPhase -= 1.0;

            const float ageScale = (tapeAge == 0) ? 0.6f : (tapeAge == 1 ? 1.0f : 1.65f);
            const float twoPi = 2.0f * 3.14159265359f;

            const float modMsL = wowFlutter * ageScale * (
                1.8f * std::sin (twoPi * static_cast<float> (wowPhase)) +
                0.55f * std::sin (twoPi * static_cast<float> (flutterPhase)));

            const float modMsR = wowFlutter * ageScale * (
                1.8f * std::cos (twoPi * static_cast<float> (wowPhase)) +
                0.55f * std::cos (twoPi * static_cast<float> (flutterPhase)));

            // Subtle +3.5ms right-channel head offset creates lush stereo spread from mono amp/IR signals
            left  = processChannel (0, left,  modMsL, 0.0f);
            right = processChannel (1, right, modMsR, 3.5f);
        }

        float processSample (float in) noexcept
        {
            float l = in, r = in;
            processStereoSample (l, r);
            return l;
        }

    private:
        struct ChannelState
        {
            std::vector<float> tapeBuffer;
            size_t writePos = 0;

            std::array<std::vector<float>, 3> springLines;
            std::array<size_t, 3> springIdx {};
            std::array<OnePoleFilter, 3> springDamp;

            BiquadFilter recPreEmphasis, playDeEmphasis, echoBass, echoTreble;
            OnePoleFilter tapeAgeLpf, fbHpf;
        };

        float processChannel (int chIdx, float in, float modMs, float stereoOffsetMs) noexcept
        {
            auto& c = channels[static_cast<size_t> (chIdx)];

            const float h1Ms = (69.0f  + smoothedRateNorm * 108.0f) + modMs + stereoOffsetMs;
            const float h2Ms = (131.0f + smoothedRateNorm * 206.0f) + modMs * 1.3f + stereoOffsetMs * 1.2f;
            const float h3Ms = (189.0f + smoothedRateNorm * 300.0f) + modMs * 1.6f + stereoOffsetMs * 1.4f;

            const float tap1 = readTapeHead (c, h1Ms);
            const float tap2 = readTapeHead (c, h2Ms);
            const float tap3 = readTapeHead (c, h3Ms);

            float headSum = 0.0f;
            bool useReverb = false;

            switch (mode)
            {
                case 0:  headSum = tap1; break;
                case 1:  headSum = tap2; break;
                case 2:  headSum = tap3; break;
                case 3:  headSum = (tap2 + tap3) * 0.72f; break;
                case 4:  headSum = tap1; useReverb = true; break;
                case 5:  headSum = tap2; useReverb = true; break;
                case 6:  headSum = tap3; useReverb = true; break;
                case 7:  headSum = (tap2 + tap3) * 0.72f; useReverb = true; break;
                case 8:  headSum = (tap1 + tap2) * 0.72f; useReverb = true; break;
                case 9:  headSum = (tap1 + tap3) * 0.72f; useReverb = true; break;
                case 10: headSum = (tap1 + tap2 + tap3) * 0.58f; useReverb = true; break;
                case 11: headSum = 0.0f; useReverb = true; break;
                default: headSum = tap2; break;
            }

            float echoReturn = c.playDeEmphasis.process (c.tapeAgeLpf.process (headSum));
            echoReturn = c.echoTreble.process (c.echoBass.process (echoReturn));

            const float hiss = (tapeAge == 2 ? 0.00018f : (tapeAge == 1 ? 0.00006f : 0.000015f))
                               * (static_cast<float> (rng() & 0xFFFF) / 32768.0f - 1.0f);
            const float recInput = c.recPreEmphasis.process (in + c.fbHpf.process (echoReturn) * intensity) + hiss;
            c.tapeBuffer[c.writePos] = std::tanh (recInput * 0.92f) * 1.06f;
            c.writePos = (c.writePos + 1) % c.tapeBuffer.size();

            float springOut = 0.0f;
            if (useReverb)
            {
                for (int i = 0; i < 3; ++i)
                {
                    const float sTap = c.springDamp[i].process (c.springLines[i][c.springIdx[i]]);
                    c.springLines[i][c.springIdx[i]] = in * 0.35f + sTap * 0.78f;
                    c.springIdx[i] = (c.springIdx[i] + 1) % c.springLines[i].size();
                    springOut += sTap;
                }
                springOut *= (0.35f * reverbLevel);
            }

            const float wet = (echoReturn * echoLevel) + springOut;
            return in * (1.0f - mix * 0.35f) + wet * mix;
        }

        float readTapeHead (const ChannelState& c, float delayMs) const noexcept
        {
            const size_t bufSize = c.tapeBuffer.size();
            const float delaySamples = std::clamp (delayMs * 0.001f * static_cast<float> (sampleRate),
                                                   1.0f, static_cast<float> (bufSize - 4));
            float rPos = static_cast<float> (c.writePos) - delaySamples;
            while (rPos < 0.0f) rPos += static_cast<float> (bufSize);

            const size_t i0 = static_cast<size_t> (rPos) % bufSize;
            const size_t i1 = (i0 + 1) % bufSize;
            const float frac = rPos - std::floor (rPos);
            return c.tapeBuffer[i0] * (1.0f - frac) + c.tapeBuffer[i1] * frac;
        }

        double sampleRate = 48000.0;
        std::array<ChannelState, 2> channels;

        int mode = 5;
        float targetRateNorm = 0.45f;
        float smoothedRateNorm = 0.45f;
        float intensity = 0.38f;
        float echoLevel = 0.75f;
        float reverbLevel = 0.35f;
        int tapeAge = 1;
        float wowFlutter = 0.25f;
        float mix = 0.35f;

        double wowPhase = 0.0;
        double flutterPhase = 0.0;
        std::minstd_rand rng { 1995u };
    };
}
