#include <cstdio>
#include <cmath>
#include <algorithm>
#include "lamp_extracted.h"

/* Alimenta a lampada real com um envelope de pico verdadeiro: leito baixo e
   estouros curtos, e conta as piscadas e quanto tempo cada uma dura. */
static void run(float burstDb, float threshDb, const char* tag)
{
    const float SR = 48000.0f, DUR = 9.0f, PERIOD = 1.5f, BURST = 0.030f;
    const float bed = std::pow(10.0f, -12.0f*0.05f);
    const float hot = std::pow(10.0f, burstDb*0.05f);

    PeakLamp lamp; lamp.configure(threshDb, SR); lamp.reset();

    int blinks = 0, on = 0, longest = 0, run_ = 0;
    bool prev = false;
    for (int i = 0; i < (int)(SR*DUR); ++i) {
        const float t = i / SR;
        lamp.tick(std::fmod(t, PERIOD) < BURST ? hot : bed);
        const bool lit = lamp.lit();
        if (lit) { ++on; ++run_; if (run_ > longest) longest = run_; }
        else run_ = 0;
        if (lit && !prev) ++blinks;
        prev = lit;
    }
    printf("%-34s  estouro %+5.1f dBTP | limiar %+5.1f dBTP -> %d piscada(s), %3.0f ms cada, %2.0f%% aceso\n",
           tag, burstDb, threshDb, blinks, longest*1000.0f/SR, 100.0f*on/(SR*DUR));
}

int main()
{
    printf("== lampada de pico: 6 estouros de 30 ms em 9 s ==\n");
    run(-5.0f, -12.0f, "estouro bem acima do limiar");
    run(-5.0f,  -6.0f, "estouro logo acima");
    run(-5.0f,  -3.0f, "estouro ABAIXO, nao pode piscar");
    run(-0.5f,  -1.0f, "ceiling de streaming");
    run(-7.9f,  -6.0f, "material do Ricardo no limiar padrao");
    run(-7.9f,  -9.0f, "mesmo material, limiar um passo abaixo");
    return 0;
}
