/*
 * ROSS VU // presets de fabrica
 *
 * Fonte unica: o DSP usa para os programas do host, a interface usa para o
 * seletor do cabecalho. Mexer aqui muda os dois.
 *
 * Indices: cal 0..6 = -20/-18/-16/-14/-12/-10/-8 dBFS
 *          mode 0..4 = L+R / LEFT / RIGHT / SIDE / L|R
 *          peak 0..5 = -12/-9/-6/-3/-1/-0.1 dBTP
 */
#pragma once

struct RossVUPreset {
    const char* name;
    float cal, response, mode, peak;
};

static const RossVUPreset kRossVUPresets[] = {
    { "MIX BUS -18",   1.0f, 0.5f, 0.0f, 2.0f },  // mixando, 300 ms, lampada -6
    { "TRACKING -20",  0.0f, 1.0f, 0.0f, 0.0f },  // gravando, agulha rapida, lampada -12
    { "MIX READY -14", 3.0f, 0.5f, 0.0f, 3.0f },  // mix pronta, lampada -3
    { "MASTER -10",    5.0f, 0.5f, 0.0f, 4.0f },  // master pronto, lampada -1
    { "REFERENCE -12", 4.0f, 0.5f, 0.0f, 4.0f },  // faixa de referencia no A/B
    { "STEREO L | R",  1.0f, 0.5f, 4.0f, 2.0f },  // dois ponteiros
    { "PROGRAM SLOW",  1.0f, 0.0f, 0.0f, 2.0f },  // 600 ms, leitura de programa
};

static const int kRossVUPresetCount = 7;
