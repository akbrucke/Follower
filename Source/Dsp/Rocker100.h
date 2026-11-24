#pragma once
#include "DspHelpers.h"
#include <cmath>
#include <algorithm>

namespace FollowerDsp
{
    /**
     * Rocker 100 (British Dual-Channel 100W EL34 Head Topology)
     * - Clean Channel: 2-stage ECC83 (V9-A -> 470pF bright -> V9-B -> 2-band Bass/Treble)
     * - Dirty Channel: 4-stage cascading ECC83 (V10-A -> V10-B -> 56pF peaking -> V8-A cold clipper -> V8-B -> 39k slope 3-band EQ)
     * - Long-Tailed Pair Phase Splitter (V5-A/V5-B) -> Quad EL34 100W Power Stage w/ 4.7k NFB
     */
    class Rocker100Amp
    {
    public:
        void prepare (double sr) noexcept
        {
            sampleRate = sr;
            inputGridLpf.setLowPass (sr, 10800.0f);

            v9a.prepare (sr, 42.0f, 8800.0f);
            v9b.prepare (sr, 36.0f, 8200.0f);

            v10a.prepare (sr, 65.0f, 8500.0f);
            v10b.prepare (sr, 78.0f, 7600.0f);
            v8a.prepare  (sr, 95.0f, 6800.0f);
            v8b.prepare  (sr, 55.0f, 7200.0f);

            piStage.prepare (sr, 32.0f, 7400.0f);
            powerStage.prepare (sr, 38.0f, 6200.0f);
            nfbLpf.setLowPass (sr, 3400.0f);
            lowMidPunch.setPeaking (sr, 210.0f, 0.75f, 2.4f);

            updateToneStack();
            reset();
        }

        void reset() noexcept
        {
            inputGridLpf.reset();
            v9a.reset(); v9b.reset();
            v10a.reset(); v10b.reset(); v8a.reset(); v8b.reset();
            piStage.reset(); powerStage.reset();
            nfbLpf.reset();
            bassFilter.reset(); midFilter.reset(); trebleFilter.reset();
            lowMidPunch.reset();
            nfbState = 0.0f;
        }

        void setParameters (float gain0to10, float treble0to10, float mid0to10,
                            float bass0to10, float volume0to10, bool dirtyChannel) noexcept
        {
            gainVal   = std::clamp (gain0to10 * 0.1f, 0.0f, 1.0f);
            trebleVal = std::clamp (treble0to10 * 0.1f, 0.0f, 1.0f);
            midVal    = std::clamp (mid0to10 * 0.1f, 0.0f, 1.0f);
            bassVal   = std::clamp (bass0to10 * 0.1f, 0.0f, 1.0f);
            volVal    = std::clamp (volume0to10 * 0.1f, 0.0f, 1.0f);
            isDirty   = dirtyChannel;

            updateToneStack();
        }

        float processSample (float in) noexcept
        {
            float x = inputGridLpf.process (in);

            if (! isDirty)
            {
                const float cleanDrive = 1.1f + std::pow (gainVal, 1.5f) * 9.5f;
                x = v9a.process (x, cleanDrive, 0.10f, 0.08f);
                x = v9b.process (x, 2.1f, 0.12f, 0.09f);
            }
            else
            {
                const float g1 = 1.8f + std::pow (gainVal, 1.35f) * 11.5f;
                const float g2 = 1.6f + std::pow (gainVal, 1.25f) * 7.5f;

                x = v10a.process (x, g1, 0.14f, 0.10f);
                x = v10b.process (x, g2, 0.18f, 0.12f);
                x = v8a.process  (x, 3.2f, 0.26f, 0.14f);
                x = v8b.process  (x, 2.2f, 0.12f, 0.10f);
            }

            x = bassFilter.process (x);
            x = midFilter.process (x);
            x = trebleFilter.process (x);

            const float masterScale = (isDirty ? 0.55f : 0.85f) * (0.4f + std::pow (volVal, 1.4f) * 5.2f);
            const float piIn = (x * masterScale) - (nfbState * 0.20f);
            const float piOut = piStage.process (piIn, 1.75f, 0.04f, 0.08f);

            const float pwr = powerStage.process (piOut, 1.95f, 0.03f, 0.15f);
            nfbState = nfbLpf.process (pwr);

            const float out = lowMidPunch.process (pwr);
            return out * (isDirty ? 0.46f : 0.58f);
        }

    private:
        void updateToneStack() noexcept
        {
            const float bassDb   = (bassVal - 0.5f) * 15.0f;
            const float midDb    = -2.2f + (midVal - 0.5f) * 14.5f;
            const float trebleDb = (trebleVal - 0.5f) * 14.5f;

            bassFilter.setLowShelf (sampleRate, 130.0f, bassDb);
            midFilter.setPeaking (sampleRate, 560.0f, 0.80f, midDb);
            trebleFilter.setHighShelf (sampleRate, 2100.0f, trebleDb);
        }

        double sampleRate = 48000.0;
        OnePoleFilter inputGridLpf, nfbLpf;
        TriodeStage v9a, v9b, v10a, v10b, v8a, v8b, piStage, powerStage;
        BiquadFilter bassFilter, midFilter, trebleFilter, lowMidPunch;

        float gainVal = 0.6f, trebleVal = 0.5f, midVal = 0.55f, bassVal = 0.55f, volVal = 0.5f;
        bool isDirty = true;
        float nfbState = 0.0f;
    };
}
