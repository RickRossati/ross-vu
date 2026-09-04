struct TruePeak {
    float coef[kOS][kTaps];
    float z[2][kTaps];
    int   idx;

    void design()
    {
        // sinc janelado por Blackman-Harris, normalizado fase a fase
        float proto[kProto];
        const double c = (kProto - 1) * 0.5;
        for (int n = 0; n < kProto; ++n)
        {
            const double t = (n - c) / static_cast<double>(kOS);
            const double s = (std::fabs(t) < 1.0e-9) ? 1.0 : std::sin(M_PI * t) / (M_PI * t);
            const double u = 2.0 * M_PI * n / (kProto - 1);
            const double w = 0.35875 - 0.48829 * std::cos(u)
                           + 0.14128 * std::cos(2.0 * u) - 0.01168 * std::cos(3.0 * u);
            proto[n] = static_cast<float>(s * w);
        }

        for (int p = 0; p < kOS; ++p)
        {
            double sum = 0.0;
            for (int k = 0; k < kTaps; ++k) sum += proto[k * kOS + p];
            const float g = (std::fabs(sum) > 1.0e-9) ? static_cast<float>(1.0 / sum) : 1.0f;
            for (int k = 0; k < kTaps; ++k) coef[p][k] = proto[k * kOS + p] * g;
        }
    }

    void reset()
    {
        idx = 0;
        for (int c = 0; c < 2; ++c)
            for (int k = 0; k < kTaps; ++k) z[c][k] = 0.0f;
    }

    // devolve o maior modulo entre as amostras interpoladas e a propria amostra
    inline float process(float l, float r)
    {
        z[0][idx] = l;
        z[1][idx] = r;

        float peak = std::fmax(std::fabs(l), std::fabs(r));

        for (int p = 0; p < kOS; ++p)
        {
            float al = 0.0f, ar = 0.0f;
            int   j  = idx;
            for (int k = 0; k < kTaps; ++k)
            {
                al += coef[p][k] * z[0][j];
                ar += coef[p][k] * z[1][j];
                if (--j < 0) j = kTaps - 1;
            }
            peak = std::fmax(peak, std::fmax(std::fabs(al), std::fabs(ar)));
        }

        if (++idx >= kTaps) idx = 0;
        return peak;
    }
};