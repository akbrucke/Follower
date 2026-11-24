#pragma once
#include <cmath>
#include <algorithm>

namespace FollowerDsp
{
    struct OnePoleFilter
    {
        float z1 = 0.0f;
        float a0 = 1.0f;
        float b1 = 0.0f;

        void reset() noexcept { z1 = 0.0f; }

        void setLowPass (double sampleRate, float cutoffHz) noexcept
        {
            const float fc = std::clamp (cutoffHz, 5.0f, static_cast<float> (sampleRate * 0.45));
            const float x  = std::exp (-2.0f * 3.14159265359f * fc / static_cast<float> (sampleRate));
            a0 = 1.0f - x;
            b1 = x;
        }

        void setHighPass (double sampleRate, float cutoffHz) noexcept
        {
            const float fc = std::clamp (cutoffHz, 2.0f, static_cast<float> (sampleRate * 0.45));
            const float x  = std::exp (-2.0f * 3.14159265359f * fc / static_cast<float> (sampleRate));
            a0 = (1.0f + x) * 0.5f;
            b1 = x;
            // Implemented via LP subtraction for phase-accurate 1-pole HPF
            lpCoeffA = 1.0f - x;
            lpCoeffB = x;
            isHighPass = true;
        }

        float process (float in) noexcept
        {
            if (isHighPass)
            {
                z1 = lpCoeffA * in + lpCoeffB * z1;
                return in - z1;
            }
            z1 = a0 * in + b1 * z1;
            return z1;
        }

    private:
        float lpCoeffA = 1.0f;
        float lpCoeffB = 0.0f;
        bool isHighPass = false;
    };

    struct BiquadFilter
    {
        float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f, a1 = 0.0f, a2 = 0.0f;
        float z1 = 0.0f, z2 = 0.0f;

        void reset() noexcept { z1 = 0.0f; z2 = 0.0f; }

        void setLowPass (double sr, float freq, float q = 0.7071f) noexcept
        {
            const float w0 = 2.0f * 3.14159265359f * std::clamp (freq, 10.0f, static_cast<float> (sr * 0.45)) / static_cast<float> (sr);
            const float cosw = std::cos (w0);
            const float sinw = std::sin (w0);
            const float alpha = sinw / (2.0f * std::max (0.1f, q));
            const float invA0 = 1.0f / (1.0f + alpha);
            b0 = ((1.0f - cosw) * 0.5f) * invA0;
            b1 = (1.0f - cosw) * invA0;
            b2 = b0;
            a1 = (-2.0f * cosw) * invA0;
            a2 = (1.0f - alpha) * invA0;
        }

        void setHighPass (double sr, float freq, float q = 0.7071f) noexcept
        {
            const float w0 = 2.0f * 3.14159265359f * std::clamp (freq, 10.0f, static_cast<float> (sr * 0.45)) / static_cast<float> (sr);
            const float cosw = std::cos (w0);
            const float sinw = std::sin (w0);
            const float alpha = sinw / (2.0f * std::max (0.1f, q));
            const float invA0 = 1.0f / (1.0f + alpha);
            b0 = ((1.0f + cosw) * 0.5f) * invA0;
            b1 = -(1.0f + cosw) * invA0;
            b2 = b0;
            a1 = (-2.0f * cosw) * invA0;
            a2 = (1.0f - alpha) * invA0;
        }

        void setPeaking (double sr, float freq, float q, float gainDb) noexcept
        {
            const float A = std::pow (10.0f, gainDb / 40.0f);
            const float w0 = 2.0f * 3.14159265359f * std::clamp (freq, 10.0f, static_cast<float> (sr * 0.45)) / static_cast<float> (sr);
            const float cosw = std::cos (w0);
            const float sinw = std::sin (w0);
            const float alpha = sinw / (2.0f * std::max (0.1f, q));
            const float invA0 = 1.0f / (1.0f + alpha / A);
            b0 = (1.0f + alpha * A) * invA0;
            b1 = (-2.0f * cosw) * invA0;
            b2 = (1.0f - alpha * A) * invA0;
            a1 = (-2.0f * cosw) * invA0;
            a2 = (1.0f - alpha / A) * invA0;
        }

        void setLowShelf (double sr, float freq, float gainDb) noexcept
        {
            const float A = std::pow (10.0f, gainDb / 40.0f);
            const float w0 = 2.0f * 3.14159265359f * std::clamp (freq, 10.0f, static_cast<float> (sr * 0.45)) / static_cast<float> (sr);
            const float cosw = std::cos (w0);
            const float sinw = std::sin (w0);
            const float alpha = sinw * 0.5f * std::sqrt ((A + 1.0f / A) * (1.0f / 0.707f - 1.0f) + 2.0f);
            const float twoSqrtAAlpha = 2.0f * std::sqrt (A) * alpha;
            const float invA0 = 1.0f / ((A + 1.0f) + (A - 1.0f) * cosw + twoSqrtAAlpha);
            b0 = (A * ((A + 1.0f) - (A - 1.0f) * cosw + twoSqrtAAlpha)) * invA0;
            b1 = (2.0f * A * ((A - 1.0f) - (A + 1.0f) * cosw)) * invA0;
            b2 = (A * ((A + 1.0f) - (A - 1.0f) * cosw - twoSqrtAAlpha)) * invA0;
            a1 = (-2.0f * ((A - 1.0f) + (A + 1.0f) * cosw)) * invA0;
            a2 = ((A + 1.0f) + (A - 1.0f) * cosw - twoSqrtAAlpha) * invA0;
        }

        void setHighShelf (double sr, float freq, float gainDb) noexcept
        {
            const float A = std::pow (10.0f, gainDb / 40.0f);
            const float w0 = 2.0f * 3.14159265359f * std::clamp (freq, 10.0f, static_cast<float> (sr * 0.45)) / static_cast<float> (sr);
            const float cosw = std::cos (w0);
            const float sinw = std::sin (w0);
            const float alpha = sinw * 0.5f * std::sqrt ((A + 1.0f / A) * (1.0f / 0.707f - 1.0f) + 2.0f);
            const float twoSqrtAAlpha = 2.0f * std::sqrt (A) * alpha;
            const float invA0 = 1.0f / ((A + 1.0f) - (A - 1.0f) * cosw + twoSqrtAAlpha);
            b0 = (A * ((A + 1.0f) + (A - 1.0f) * cosw + twoSqrtAAlpha)) * invA0;
            b1 = (-2.0f * A * ((A - 1.0f) + (A + 1.0f) * cosw)) * invA0;
            b2 = (A * ((A + 1.0f) + (A - 1.0f) * cosw - twoSqrtAAlpha)) * invA0;
            a1 = (2.0f * ((A - 1.0f) - (A + 1.0f) * cosw)) * invA0;
            a2 = ((A + 1.0f) - (A - 1.0f) * cosw - twoSqrtAAlpha) * invA0;
        }

        float process (float in) noexcept
        {
            const float out = b0 * in + z1;
            z1 = b1 * in - a1 * out + z2;
            z2 = b2 * in - a2 * out;
            return out;
        }
    };

    // Triode stage modeled with asymmetric grid conduction, cathode compression, and Miller LPF
    struct TriodeStage
    {
        OnePoleFilter couplingHpf;
        OnePoleFilter millerLpf;
        float cathodeEnv = 0.0f;
        float attCoeff = 0.01f;
        float relCoeff = 0.001f;

        void prepare (double sr, float hpfHz, float millerHz) noexcept
        {
            couplingHpf.setHighPass (sr, hpfHz);
            millerLpf.setLowPass (sr, millerHz);
            attCoeff = 1.0f - std::exp (-1.0f / (static_cast<float> (sr) * 0.003f));
            relCoeff = 1.0f - std::exp (-1.0f / (static_cast<float> (sr) * 0.045f));
            reset();
        }

        void reset() noexcept
        {
            couplingHpf.reset();
            millerLpf.reset();
            cathodeEnv = 0.0f;
        }

        float process (float x, float driveGain, float asymBias = 0.14f, float sagAmount = 0.12f) noexcept
        {
            float v = couplingHpf.process (x) * driveGain;
            const float absV = std::abs (v);
            cathodeEnv += (absV > cathodeEnv ? attCoeff : relCoeff) * (absV - cathodeEnv);

            // Dynamic bias shift under heavy grid conduction
            const float bias = asymBias + cathodeEnv * sagAmount;
            const float shifted = v + bias;

            // Asymmetric 12AX7/ECC83 transfer curve: soft grid clipping on positive half, harder cutoff on negative
            float plate;
            if (shifted >= 0.0f)
                plate = std::tanh (shifted * 1.15f) * 0.88f;
            else
                plate = std::tanh (shifted * 0.82f) * 1.08f;

            // Remove static DC shift induced by asymBias
            const float dcOffset = std::tanh (asymBias * 1.15f) * 0.88f;
            return millerLpf.process (plate - dcOffset);
        }
    };
}
