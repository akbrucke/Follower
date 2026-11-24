#pragma once
#include "DspHelpers.h"
#include <cmath>
#include <algorithm>

namespace FollowerDsp
{
    /**
     * Custom Blues-Style Overdrive
     * - TO-92 discrete input buffer
     * - Dual MC1458P op-amp stages with slew-rate limiting (~0.5 V/us warm roll-off)
     * - 5mm Red LED clipping stage (~1.75V forward threshold)
     * - Tweak 1: FATNESS (variable pre-clip bass HPF from tight 310Hz down to 42Hz weight)
     * - Tweak 2: VOLTAGE (4.5V sagged/starved rails -> 9.0V stock -> 18.0V high-headroom boost)
     */
    class OverdrivePedal
    {
    public:
        void prepare (double sr) noexcept
        {
            sampleRate = sr;
            inputHpf.setHighPass (sr, 28.0f);
            fatnessHpf.setHighPass (sr, 120.0f);
            slewStage1.setLowPass (sr, 6800.0f);
            slewStage2.setLowPass (sr, 5400.0f);
            toneLpf.setLowPass (sr, 2400.0f);
            outputHpf.setHighPass (sr, 22.0f);
            reset();
        }

        void reset() noexcept
        {
            inputHpf.reset();
            fatnessHpf.reset();
            slewStage1.reset();
            slewStage2.reset();
            toneLpf.reset();
            outputHpf.reset();
            sagEnv = 0.0f;
            lastSlewOut = 0.0f;
        }

        void setParameters (float gain0to10, float tone0to10, float level0to10,
                            float fatness0to10, float voltage45to18) noexcept
        {
            const float gNorm = std::clamp (gain0to10 * 0.1f, 0.0f, 1.0f);
            driveGain = 1.5f + std::pow (gNorm, 1.65f) * 46.5f;

            const float fNorm = std::clamp (fatness0to10 * 0.1f, 0.0f, 1.0f);
            const float fatFreq = 310.0f * std::pow (42.0f / 310.0f, fNorm);
            fatnessHpf.setHighPass (sampleRate, fatFreq);

            const float tNorm = std::clamp (tone0to10 * 0.1f, 0.0f, 1.0f);
            const float toneFreq = 750.0f * std::pow (5200.0f / 750.0f, tNorm);
            toneLpf.setLowPass (sampleRate, toneFreq);

            supplyVoltage = std::clamp (voltage45to18, 4.5f, 18.0f);

            const float lNorm = std::clamp (level0to10 * 0.1f, 0.0f, 1.0f);
            outputGain = (lNorm <= 0.001f) ? 0.0f : (lNorm * 2.2f);
        }

        float processSample (float in) noexcept
        {
            float x = inputHpf.process (in);
            x = fatnessHpf.process (x);

            const float voltRatio = supplyVoltage / 9.0f;
            const float sagFactor = std::max (0.0f, (10.5f - supplyVoltage) / 6.0f);
            const float absIn = std::abs (x * driveGain);
            sagEnv = 0.996f * sagEnv + 0.004f * absIn;
            const float effectiveRail = std::max (0.28f, voltRatio * (1.0f - sagFactor * 0.38f * std::tanh (sagEnv)));

            float s1 = x * driveGain;
            const float maxDelta = (0.35f * effectiveRail) * (48000.0f / static_cast<float> (sampleRate));
            const float delta = std::clamp (s1 - lastSlewOut, -maxDelta, maxDelta);
            lastSlewOut += delta;
            s1 = slewStage1.process (lastSlewOut);

            const float ledThreshold = 1.45f * effectiveRail;
            const float normalizedToLed = s1 / ledThreshold;
            const float clipped = ledThreshold * (normalizedToLed / std::pow (1.0f + std::pow (std::abs (normalizedToLed), 2.6f), 1.0f / 2.6f));

            float s2 = slewStage2.process (clipped * 1.18f);
            s2 = toneLpf.process (s2);

            return outputHpf.process (s2) * outputGain;
        }

    private:
        double sampleRate = 48000.0;
        OnePoleFilter inputHpf, fatnessHpf, slewStage1, slewStage2, toneLpf, outputHpf;
        float driveGain = 8.0f;
        float supplyVoltage = 9.0f;
        float outputGain = 1.0f;
        float sagEnv = 0.0f;
        float lastSlewOut = 0.0f;
    };
}
