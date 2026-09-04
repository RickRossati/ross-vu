#include <cstdio>
#include <cmath>
#include <algorithm>
static const int kOS = 4, kTaps = 12, kProto = kOS * kTaps;
#include "tp_extracted.h"

static void check(double freqRatio, double phase, double amp, const char* tag) {
    TruePeak tp; tp.design(); tp.reset();
    const double w = 2.0 * M_PI * freqRatio;
    double samplePk = 0.0, truePk = 0.0;
    for (int n = 0; n < 4096; ++n) {
        const float x = (float)(amp * sin(w * n + phase));
        samplePk = std::max(samplePk, (double)fabsf(x));
        const float p = tp.process(x, x);
        if (n > 64) truePk = std::max(truePk, (double)p);   // depois do transitorio
    }
    const double realPk = amp;  // pico analitico da senoide
    printf("%-30s  amostra %+6.2f dBFS | ROSS VU %+6.2f dBTP | real %+6.2f dB | erro %+.2f dB\n",
           tag, 20*log10(samplePk), 20*log10(truePk), 20*log10(realPk),
           20*log10(truePk) - 20*log10(realPk));
}

int main() {
    printf("== pico entre amostras: o pico de amostra mente, o dBTP nao pode ==\n");
    check(0.25,  M_PI/4, 1.0,      "fs/4 fase 45 (caso classico)");
    check(0.25,  M_PI/4, 0.891251, "fs/4 fase 45 em -1 dB");
    check(0.30,  0.4,    1.0,      "0.30 fs fase qualquer");
    check(0.40,  0.7,    1.0,      "0.40 fs (perto de Nyquist)");
    check(0.05,  0.3,    1.0,      "0.05 fs (grave, deve bater)");
    check(0.011, 0.0,    0.5,      "1 kHz em 90k a -6 dB");
    return 0;
}
