#pragma once
#include <cmath>
#include <algorithm>
#include <vector>

// 8 frames de onda (seno -> triangulo -> saw -> ... -> digital) generadas por sintesis aditiva.
// Cada frame tiene 9 versiones "mipmap" con menos armonicos para evitar aliasing en notas agudas.
struct Wavetable
{
    static constexpr int N = 2048;
    static constexpr int frames = 8;
    static constexpr int mips = 9;
    static constexpr int maxHarm = 512;

    static const Wavetable& get()
    {
        static Wavetable w;
        return w;
    }

    // pos: 0..1 (morph entre frames), phase: 0..1
    float read(float pos, int mip, float phase) const noexcept
    {
        const float fp = pos * (float)(frames - 1);
        const int f0 = std::min((int)fp, frames - 1);
        const int f1 = std::min(f0 + 1, frames - 1);
        const float ff = fp - (float)f0;

        const float x = phase * (float)N;
        const int i = std::min((int)x, N - 1);
        const float fr = x - (float)i;

        const float a = data[f0][mip][i] * (1.0f - fr) + data[f0][mip][i + 1] * fr;
        const float b = data[f1][mip][i] * (1.0f - fr) + data[f1][mip][i + 1] * fr;
        return a * (1.0f - ff) + b * ff;
    }

    static int mipFor(float freq, float sampleRate) noexcept
    {
        const float allowed = 0.5f * sampleRate / std::max(freq, 1.0f);
        int m = 0;
        while (m < mips - 1 && (float)(maxHarm >> m) > allowed)
            ++m;
        return m;
    }

private:
    static double harmAmp(int f, int h)
    {
        const double hd = (double)h;
        const double pi = 3.14159265358979323846;
        switch (f)
        {
            case 0: return h == 1 ? 1.0 : 0.0;                                   // seno
            case 1: return (h & 1) ? ((((h - 1) / 2) & 1) ? -1.0 : 1.0) / (hd * hd) : 0.0; // triangulo
            case 2: return 1.0 / hd;                                             // saw
            case 3: return 1.0 / std::pow(hd, 0.7);                              // saw brillante
            case 4: return (h & 1) ? 1.0 / hd : 0.0;                             // cuadrada
            case 5: return std::sin(pi * hd * 0.25) / hd;                        // pulso 25%
            case 6: return (1.0 / hd) * (1.0 + 4.0 * std::exp(-((hd - 5.0) * (hd - 5.0)) / 6.0)); // formante
            default: return 1.0 / std::sqrt(hd);                                 // digital/cristal
        }
    }

    Wavetable()
    {
        std::vector<float> sinT((size_t)N);
        for (int i = 0; i < N; ++i)
            sinT[(size_t)i] = (float)std::sin(2.0 * 3.14159265358979323846 * (double)i / (double)N);

        for (int f = 0; f < frames; ++f)
        {
            for (int m = 0; m < mips; ++m)
            {
                const int maxH = maxHarm >> m;
                float peak = 0.0f;
                for (int i = 0; i < N; ++i)
                {
                    double sum = 0.0;
                    for (int h = 1; h <= maxH; ++h)
                    {
                        const double a = harmAmp(f, h);
                        if (a != 0.0)
                            sum += a * (double)sinT[(size_t)((h * i) & (N - 1))];
                    }
                    data[f][m][i] = (float)sum;
                    peak = std::max(peak, std::abs((float)sum));
                }
                const float norm = peak > 0.0f ? 1.0f / peak : 1.0f;
                for (int i = 0; i < N; ++i)
                    data[f][m][i] *= norm;
                data[f][m][N] = data[f][m][0];
            }
        }
    }

    float data[frames][mips][N + 1];
};
