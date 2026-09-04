#pragma once

#define DISTRHO_PLUGIN_BRAND       "Mister RickRoss"
#define DISTRHO_PLUGIN_NAME        "ROSS VU"
#define DISTRHO_PLUGIN_URI         "https://chaosarchitect.art/plugins/ross-vu"
#define DISTRHO_PLUGIN_CLAP_ID     "art.chaosarchitect.ross-vu"
#define DISTRHO_PLUGIN_BRAND_ID    MrRs
#define DISTRHO_PLUGIN_UNIQUE_ID   RsVu

#define DISTRHO_PLUGIN_HAS_UI        1
#define DISTRHO_PLUGIN_IS_RT_SAFE    1
#define DISTRHO_PLUGIN_WANT_PROGRAMS 1
#define DISTRHO_PLUGIN_NUM_INPUTS    2
#define DISTRHO_PLUGIN_NUM_OUTPUTS   2

#define DISTRHO_UI_USE_NANOVG        1
#define DISTRHO_UI_USER_RESIZABLE    1
#define DISTRHO_UI_DEFAULT_WIDTH     1000
#define DISTRHO_UI_DEFAULT_HEIGHT    640

#define DISTRHO_PLUGIN_LV2_CATEGORY  "lv2:AnalyserPlugin"
#define DISTRHO_PLUGIN_VST3_CATEGORIES "Fx|Analyzer"
#define DISTRHO_PLUGIN_CLAP_FEATURES "audio-effect", "analyzer", "stereo"

enum Parameters {
    // entrada
    kParamInput = 0,
    kParamOutput,
    kParamResponse,
    kParamCalibration,
    kParamMode,
    kParamPeakRef,
    // saida (o DSP escreve, a UI le)
    kOutDeflection,
    kOutDeflection2,
    kOutVu,
    kOutVu2,
    kOutTruePeak,
    kOutLed,
    kParameterCount
};

// escala do mostrador: fundo de escala = +3 VU
#define ROSSVU_FULLSCALE_VU 3.0f
