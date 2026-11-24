#pragma once
#include "DspHelpers.h"
#include <cmath>
#include <algorithm>

namespace FollowerDsp
{
    /**
     * Trainer Mark III (Canadian High-Headroom 100W Tube Head Topology)
     * - V1A/V2A 12AX7A input stage (68k grid stopper, 1.5k//25uF cathode, 100k plate)
     * - Passive 3-band tone stack (250k Treble, 1M Bass, 25k Mid, 270pF/0.047uF, 100k slope)
     * - Switchable Treble Boost network (250k + 0.001uF peaking shelf)
     * - V1B/V2B recovery stage (820R unbypassed cathode, 100k plate)
     * - Master Volume (100k Lin) -> V6A/V6B Long-Tailed Pair Phase Inverter
     * - Quad 6CA7 / EL34 Class AB Push-Pull Power Stage w/ 1k/100R global NFB
     */
    class TrainerMarkIIIAmp
    {
    public:
        void prepare (double sr) noexcept
        {
            sampleRate = sr;
            inputGridLpf.setLowPass (sr, 11500.0f);
            v1a.prepare (sr, 38.0f, 9200.0f);
            v1b.prepare (sr, 32.0f, 8500.0f);
            piStage.prepare (sr, 28.0f, 7800.0f);
            powerStage.prepare (sr, 35.0f, 6500.0f);
            nfbLpf.setLowPass (sr, 3800.0f);
            presenceShelf.setHighShelf (sr, 2600.0f, 2.2f);
            updateToneStack();
            reset();
        }

        void reset() noexcept
        {
            inputGridLpf.reset();
            v1a.reset();
            v1b.reset();
            piStage.reset();
            powerStage.reset();
            bassFilter.reset();
            midFilter.reset();
            trebleFilter.reset();
            brightShelf.reset();
            nfbLpf.reset();
            presenceShelf.reset();
            nfbState = 0.0f;
        }

        void setParameters (float gain0to10, float treble0to10, float mid0to10,
                            float bass0to10, float master0to10, bool trebleBoost) noexcept
        {
            const float g = std::clamp (gain0to10 * 0.1f, 0.0f, 1.0f);
            preampDrive = 0.85f + std::pow (g, 1.55f) * 14.5f;

            const float m = std::clamp (master0to10 * 0.1f, 0.0f, 1.0f);
            masterDrive = 0.45f + std::pow (m, 1.45f) * 6.8f;

            trebleVal = std::clamp (treble0to10 * 0.1f, 0.0f, 1.0f);
            midVal    = std::clamp (mid0to10 * 0.1f, 0.0f, 1.0f);
            bassVal   = std::clamp (bass0to10 * 0.1f, 0.0f, 1.0f);
            boostOn   = trebleBoost;

            updateToneStack();
        }

        float processSample (float in) noexcept
        {
            float x = inputGridLpf.process (in);
            x = v1a.process (x, preampDrive, 0.12f, 0.10f);

            x = bassFilter.process (x);
            x = midFilter.process (x);
            x = trebleFilter.process (x);
            if (boostOn)
                x = brightShelf.process (x);

            x = v1b.process (x, 2.35f, 0.08f, 0.06f);

            const float piIn = (x * masterDrive) - (nfbState * 0.24f);
            const float piOut = piStage.process (piIn, 1.65f, 0.05f, 0.08f);

            const float pwr = powerStage.process (piOut, 1.85f, 0.03f, 0.16f);
            nfbState = nfbLpf.process (pwr);

            const float out = presenceShelf.process (pwr);
            return out * 0.56f;
        }

    private:
        void updateToneStack() noexcept
        {
            const float bassDb   = (bassVal - 0.5f) * 14.0f;
            const float midDb    = -5.5f + (midVal - 0.5f) * 13.0f;
            const float trebleDb = (trebleVal - 0.5f) * 15.0f;

            bassFilter.setLowShelf (sampleRate, 115.0f, bassDb);
            midFilter.setPeaking (sampleRate, 480.0f, 0.85f, midDb);
            trebleFilter.setHighShelf (sampleRate, 1850.0f, trebleDb);
            brightShelf.setPeaking (sampleRate, 2400.0f, 0.9f, 5.2f);
        }

        double sampleRate = 48000.0;
        OnePoleFilter inputGridLpf, nfbLpf;
        TriodeStage v1a, v1b, piStage, powerStage;
        BiquadFilter bassFilter, midFilter, trebleFilter, brightShelf, presenceShelf;

        float preampDrive = 2.5f;
        float masterDrive = 1.8f;
        float trebleVal = 0.5f, midVal = 0.5f, bassVal = 0.5f;
        bool boostOn = false;
        float nfbState = 0.0f;
    };
}
