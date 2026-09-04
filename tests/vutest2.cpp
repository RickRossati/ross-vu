#include <cstdio>
#include <cmath>
#include "mov_extracted.h"
static const float kSineFormFactor=1.110721f,kZeta=0.826f,kOmegaRef=14.0f,kRefSec=0.300f;

static void run(float sr,float knob,float calDb,float toneDb,const char* tag){
    const float t=(knob<=0.5f)?(0.600f-0.600f*knob):(0.300f-0.400f*(knob-0.5f));
    const float w=kOmegaRef*(kRefSec/fmaxf(t,0.050f));
    const float w2=w*w, damping=2.0f*kZeta*w, dt=1.0f/sr;
    const float refScale=kSineFormFactor/powf(10.0f,calDb*0.05f);
    Movement m; m.reset();
    const float amp=powf(10.0f,toneDb*0.05f)*sqrtf(2.0f), wt=2.0f*3.14159265f*1000.0f/sr;
    float t99=-1,pk=0,settle=0; const int n=(int)(sr*3.0f);
    for(int i=0;i<n;++i){ m.tick(fabsf(amp*sinf(wt*i))*refScale,w2,damping,dt);
        if(t99<0&&m.pos>=0.99f)t99=i/sr; if(m.pos>pk)pk=m.pos; if(i>(int)(sr*2.5f))settle=m.pos; }
    printf("%-24s -> repouso %+.3f VU | 99%% em %.0f ms | overshoot %.2f%%\n",
           tag,20*log10f(settle),t99*1000.0f,(pk-1.0f)*100.0f);
}
int main(){
    printf("== balistica depois da refatoracao ==\n");
    run(48000,0.5f,-18,-18,"48k CAL-18 meio"); run(44100,0.5f,-18,-18,"44.1k CAL-18 meio");
    run(96000,0.5f,-18,-18,"96k CAL-18 meio"); run(48000,0.5f,-14,-14,"48k CAL-14");
    printf("\n== calibracoes novas: senoide no proprio nivel deve dar 0.000 VU ==\n");
    run(48000,0.5f,-10,-10,"CAL-10, tom -10");
    run(48000,0.5f, -8, -8,"CAL-8,  tom -8");
    printf("\n== o MESMO master de -14 dBFS RMS em cada calibracao ==\n");
    run(48000,0.5f,-18,-14,"CAL-18");
    run(48000,0.5f,-14,-14,"CAL-14");
    run(48000,0.5f,-10,-14,"CAL-10");
    printf("\n== response ==\n");
    run(48000,0.0f,-18,-18,"SLOW 600 ms");     run(48000,1.0f,-18,-18,"FAST 100 ms");
    return 0;
}
