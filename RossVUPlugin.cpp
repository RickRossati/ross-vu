/*
 * ROSS VU // ANALOG ENERGY METER
 *
 * Balistica de VU de verdade: retificacao de media + movimento de segunda ordem.
 * zeta = 0.826 e w0 = 14 rad/s dao 99% da deflexao em 300 ms com ~1% de overshoot,
 * que e a definicao normativa do instrumento VU.
 *
 * Pico verdadeiro por sobreamostragem 4x polifasica (12 coeficientes por fase),
 * no espirito da ITU-R BS.1770-4.
 */
#include "DistrhoPlugin.hpp"
#include "RossVUPresets.h"

#include <algorithm>
#include <cmath>

START_NAMESPACE_DISTRHO

// -20 a -12 sao calibracoes de gravacao e mixagem. -10 e -8 existem para
// referenciar master pronto, que fica 4 a 6 dB acima de qualquer uma delas.
static const float kCalTable[7] = { -20.0f, -18.0f, -16.0f, -14.0f, -12.0f, -10.0f, -8.0f };

// limiar da lampada de pico, em dBTP. Uma lampada que so acende no clipe nao
// serve para nada: o trabalho dela e mostrar o transiente que a agulha lenta esconde.
static const float kPeakRefTable[6] = { -12.0f, -9.0f, -6.0f, -3.0f, -1.0f, -0.1f };

// media de |sin| = 0.9003 * RMS. O fator inverso calibra o movimento para que
// uma senoide no nivel de referencia pare exatamente em 0 VU.
static const float kSineFormFactor = 1.110721f;

static const float kZeta       = 0.826f;   // ~1% de overshoot
static const float kOmegaRef   = 14.0f;    // rad/s para os 300 ms normativos
static const float kRefSeconds = 0.300f;

static const int kOS    = 4;   // fator de sobreamostragem do pico verdadeiro
static const int kTaps  = 12;  // coeficientes por fase
static const int kProto = kOS * kTaps;

// -------------------------------------------------------- movimento do ponteiro
struct Movement {
    float pos, vel;

    void reset() { pos = vel = 0.0f; }

    inline void tick(float drive, float w2, float damping, float dt)
    {
        vel += (w2 * (drive - pos) - damping * vel) * dt;
        pos += vel * dt;
        if (pos < 0.0f) { pos = 0.0f; if (vel < 0.0f) vel = 0.0f; }
        else if (pos > 2.5f) { pos = 2.5f; }
    }
};

// ------------------------------------------------------------ lampada de pico
struct PeakLamp {
    float thresh;
    int   holdLen, hold;

    void configure(float dbtp, float sampleRate)
    {
        thresh  = std::pow(10.0f, dbtp * 0.05f);
        holdLen = static_cast<int>(sampleRate * 0.40f);   // piscada curta, legivel
        if (holdLen < 1) holdLen = 1;
    }

    void reset() { hold = 0; }

    inline void tick(float tp)
    {
        if (tp >= thresh) hold = holdLen;
        else if (hold > 0) --hold;
    }

    bool lit() const { return hold > 0; }
};

// ------------------------------------------------------ interpolador polifasico
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

// ------------------------------------------------------------------- o plugin
class RossVUPlugin final : public Plugin
{
public:
    RossVUPlugin()
        : Plugin(kParameterCount, kProgramCount, 0),
          fInput(0.0f), fOutput(0.0f), fResponse(0.5f), fCal(1.0f), fMode(0.0f), fPeakRef(2.0f)
    {
        fSampleRate = static_cast<float>(getSampleRate());
        fTp.design();
        recalc();
        resetState();
    }

protected:
    enum { kProgramCount = kRossVUPresetCount };

    const char* getLabel()       const override { return "ROSSVU"; }
    const char* getDescription() const override
    {
        return "Medidor VU analogico com balistica normativa de 300 ms, calibracao "
               "em dBFS, pico verdadeiro por sobreamostragem 4x e estagio de ganho.";
    }
    const char* getMaker()   const override { return "Mister RickRoss"; }
    const char* getHomePage()const override { return "https://chaosarchitect.art"; }
    const char* getLicense() const override { return "GPL-3.0-or-later"; }
    uint32_t    getVersion() const override { return d_version(1, 4, 2); }
    int64_t     getUniqueId()const override { return d_cconst('M','R','V','U'); }

    void initAudioPort(bool input, uint32_t index, AudioPort& port) override
    {
        port.groupId = kPortGroupStereo;
        Plugin::initAudioPort(input, index, port);
    }

    // ------------------------------------------------------------- programas
    void initProgramName(uint32_t index, String& name) override
    {
        name = kRossVUPresets[index < kProgramCount ? index : 0].name;
    }

    void loadProgram(uint32_t index) override
    {
        const RossVUPreset& p = kRossVUPresets[index < kProgramCount ? index : 0];
        fCal      = p.cal;
        fResponse = p.response;
        fMode     = p.mode;
        fPeakRef  = p.peak;
        fInput    = 0.0f;
        fOutput   = 0.0f;
        recalc();
    }

    // ------------------------------------------------------------ parametros
    void initParameter(uint32_t index, Parameter& p) override
    {
        switch (index)
        {
        case kParamInput:
            p.hints  = kParameterIsAutomatable;
            p.name   = "Input";  p.symbol = "input";  p.unit = "dB";
            p.ranges.def = 0.0f; p.ranges.min = -24.0f; p.ranges.max = 24.0f;
            break;

        case kParamOutput:
            p.hints  = kParameterIsAutomatable;
            p.name   = "Output"; p.symbol = "output"; p.unit = "dB";
            p.ranges.def = 0.0f; p.ranges.min = -24.0f; p.ranges.max = 24.0f;
            break;

        case kParamResponse:
            p.hints  = kParameterIsAutomatable;
            p.name   = "Response"; p.symbol = "response";
            p.ranges.def = 0.5f; p.ranges.min = 0.0f; p.ranges.max = 1.0f;
            break;

        case kParamCalibration:
            p.hints  = kParameterIsAutomatable | kParameterIsInteger;
            p.name   = "Calibration"; p.symbol = "calibration"; p.unit = "dBFS";
            p.ranges.def = 1.0f; p.ranges.min = 0.0f; p.ranges.max = 6.0f;
            p.enumValues.count = 7;
            p.enumValues.restrictedMode = true;
            {
                ParameterEnumerationValue* const v = new ParameterEnumerationValue[7];
                v[0].value = 0.0f; v[0].label = "-20 dBFS";
                v[1].value = 1.0f; v[1].label = "-18 dBFS";
                v[2].value = 2.0f; v[2].label = "-16 dBFS";
                v[3].value = 3.0f; v[3].label = "-14 dBFS";
                v[4].value = 4.0f; v[4].label = "-12 dBFS";
                v[5].value = 5.0f; v[5].label = "-10 dBFS";
                v[6].value = 6.0f; v[6].label = "-8 dBFS";
                p.enumValues.values = v;
            }
            break;

        case kParamMode:
            p.hints  = kParameterIsAutomatable | kParameterIsInteger;
            p.name   = "Mode"; p.symbol = "mode";
            p.ranges.def = 0.0f; p.ranges.min = 0.0f; p.ranges.max = 4.0f;
            p.enumValues.count = 5;
            p.enumValues.restrictedMode = true;
            {
                ParameterEnumerationValue* const v = new ParameterEnumerationValue[5];
                v[0].value = 0.0f; v[0].label = "L+R";
                v[1].value = 1.0f; v[1].label = "Left";
                v[2].value = 2.0f; v[2].label = "Right";
                v[3].value = 3.0f; v[3].label = "Side";
                v[4].value = 4.0f; v[4].label = "L | R";
                p.enumValues.values = v;
            }
            break;

        case kParamPeakRef:
            p.hints  = kParameterIsAutomatable | kParameterIsInteger;
            p.name   = "Peak Threshold"; p.symbol = "peak_ref"; p.unit = "dBTP";
            p.ranges.def = 2.0f; p.ranges.min = 0.0f; p.ranges.max = 5.0f;
            p.enumValues.count = 6;
            p.enumValues.restrictedMode = true;
            {
                ParameterEnumerationValue* const v = new ParameterEnumerationValue[6];
                v[0].value = 0.0f; v[0].label = "-12 dBTP";
                v[1].value = 1.0f; v[1].label = "-9 dBTP";
                v[2].value = 2.0f; v[2].label = "-6 dBTP";
                v[3].value = 3.0f; v[3].label = "-3 dBTP";
                v[4].value = 4.0f; v[4].label = "-1 dBTP";
                v[5].value = 5.0f; v[5].label = "-0.1 dBTP";
                p.enumValues.values = v;
            }
            break;

        case kOutDeflection:
        case kOutDeflection2:
            p.hints = kParameterIsOutput;
            p.name   = index == kOutDeflection ? "Needle"  : "Needle R";
            p.symbol = index == kOutDeflection ? "needle"  : "needle_r";
            p.ranges.def = 0.0f; p.ranges.min = 0.0f; p.ranges.max = 2.0f;
            break;

        case kOutVu:
        case kOutVu2:
            p.hints = kParameterIsOutput;
            p.name   = index == kOutVu ? "VU" : "VU R";
            p.symbol = index == kOutVu ? "vu" : "vu_r";
            p.unit = "VU";
            p.ranges.def = -40.0f; p.ranges.min = -40.0f; p.ranges.max = 6.0f;
            break;

        case kOutTruePeak:
            p.hints = kParameterIsOutput;
            p.name  = "True Peak"; p.symbol = "true_peak"; p.unit = "dBTP";
            p.ranges.def = -90.0f; p.ranges.min = -90.0f; p.ranges.max = 12.0f;
            break;

        case kOutLed:
            p.hints = kParameterIsOutput;
            p.name  = "Peak LED"; p.symbol = "peak_led";
            p.ranges.def = 0.0f; p.ranges.min = 0.0f; p.ranges.max = 1.0f;
            break;
        }
    }

    float getParameterValue(uint32_t index) const override
    {
        switch (index)
        {
        case kParamInput:       return fInput;
        case kParamOutput:      return fOutput;
        case kParamResponse:    return fResponse;
        case kParamCalibration: return fCal;
        case kParamMode:        return fMode;
        case kParamPeakRef:     return fPeakRef;
        case kOutDeflection:    return fNeedle;
        case kOutDeflection2:   return fNeedle2;
        case kOutVu:            return fVuOut;
        case kOutVu2:           return fVuOut2;
        case kOutTruePeak:      return fTpOut;
        case kOutLed:           return fLedOut;
        }
        return 0.0f;
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index)
        {
        case kParamInput:       fInput    = value; break;
        case kParamOutput:      fOutput   = value; break;
        case kParamResponse:    fResponse = value; break;
        case kParamCalibration: fCal      = value; break;
        case kParamMode:        fMode     = value; break;
        case kParamPeakRef:     fPeakRef  = value; break;
        default: return;
        }
        recalc();
    }

    void sampleRateChanged(double newRate) override
    {
        fSampleRate = static_cast<float>(newRate);
        recalc();
        resetState();
    }

    void activate() override { resetState(); }

    // ------------------------------------------------------------------ audio
    void run(const float** inputs, float** outputs, uint32_t frames) override
    {
        const float* inL  = inputs[0];
        const float* inR  = inputs[1];
        float*       outL = outputs[0];
        float*       outR = outputs[1];

        const int   mode      = static_cast<int>(fMode + 0.5f);
        const float dt        = fInvSampleRate;
        const float w2        = fOmega * fOmega;
        const float damping   = 2.0f * kZeta * fOmega;
        const float targetIn  = std::pow(10.0f, fInput  * 0.05f);
        const float targetOut = std::pow(10.0f, fOutput * 0.05f);

        Movement  a = fMovA, b = fMovB;
        PeakLamp  lamp = fLamp;
        float gIn = fGainIn, gOut = fGainOut, peak = fPeakLin;
        int   hold = fPeakHold;

        for (uint32_t i = 0; i < frames; ++i)
        {
            gIn  += (targetIn  - gIn ) * fGainSmooth;
            gOut += (targetOut - gOut) * fGainSmooth;

            const float l = inL[i] * gIn;
            const float r = inR[i] * gIn;

            // sinal medido, conforme o modo
            float m1, m2;
            switch (mode)
            {
            case 1:  m1 = l;              m2 = m1; break;
            case 2:  m1 = r;              m2 = m1; break;
            case 3:  m1 = (l - r) * 0.5f; m2 = m1; break;
            case 4:  m1 = l;              m2 = r;  break;
            default: m1 = (l + r) * 0.5f; m2 = m1; break;
            }

            a.tick(std::fabs(m1) * fRefScale, w2, damping, dt);
            b.tick(std::fabs(m2) * fRefScale, w2, damping, dt);

            // pico verdadeiro, sobreamostrado 4x, com retencao e queda
            const float tp = fTp.process(l, r);
            if (tp > peak) { peak = tp; hold = fPeakHoldLen; }
            else if (--hold <= 0) { peak *= fPeakDecay; hold = 0; }

            lamp.tick(tp);

            outL[i] = l * gOut;
            outR[i] = r * gOut;
        }

        fMovA     = a;
        fMovB     = b;
        fGainIn   = gIn;
        fGainOut  = gOut;
        fPeakLin  = peak;
        fPeakHold = hold;
        fLamp     = lamp;

        fNeedle  = a.pos;
        fNeedle2 = b.pos;
        fVuOut   = 20.0f * std::log10(std::fmax(a.pos, 1.0e-4f));
        fVuOut2  = 20.0f * std::log10(std::fmax(b.pos, 1.0e-4f));
        fTpOut   = 20.0f * std::log10(std::fmax(peak,  3.2e-5f));
        fLedOut  = lamp.lit() ? 1.0f : 0.0f;
    }

private:
    void recalc()
    {
        fInvSampleRate = 1.0f / fSampleRate;

        // tempo de integracao: 600 ms no minimo, 300 ms exatos ao meio, 100 ms no maximo
        const float k = std::fmin(std::fmax(fResponse, 0.0f), 1.0f);
        const float t = (k <= 0.5f) ? (0.600f - 0.600f * k)
                                    : (0.300f - 0.400f * (k - 0.5f));
        fOmega = kOmegaRef * (kRefSeconds / std::fmax(t, 0.050f));

        const int   ci  = std::min(std::max(static_cast<int>(fCal + 0.5f), 0), 6);
        const float ref = std::pow(10.0f, kCalTable[ci] * 0.05f);
        fRefScale = kSineFormFactor / ref;

        const int pi = std::min(std::max(static_cast<int>(fPeakRef + 0.5f), 0), 5);
        fLamp.configure(kPeakRefTable[pi], fSampleRate);

        fGainSmooth  = 1.0f - std::exp(-fInvSampleRate / 0.020f);
        fPeakHoldLen = static_cast<int>(fSampleRate * 1.5f);
        fPeakDecay   = std::exp(-fInvSampleRate * 2.303f * 1.5f); // ~30 dB/s
    }

    void resetState()
    {
        fMovA.reset();
        fMovB.reset();
        fTp.reset();
        fGainIn   = std::pow(10.0f, fInput  * 0.05f);
        fGainOut  = std::pow(10.0f, fOutput * 0.05f);
        fLamp.reset();
        fPeakLin  = 0.0f;
        fPeakHold = 0;
        fNeedle   = fNeedle2 = 0.0f;
        fVuOut    = fVuOut2  = -40.0f;
        fTpOut    = -90.0f;
        fLedOut   = 0.0f;
    }

    float fInput, fOutput, fResponse, fCal, fMode, fPeakRef;
    float fNeedle, fNeedle2, fVuOut, fVuOut2, fTpOut, fLedOut;

    float fSampleRate, fInvSampleRate;
    float fOmega, fRefScale, fGainSmooth, fPeakDecay;
    int   fPeakHoldLen;

    Movement fMovA, fMovB;
    TruePeak fTp;
    PeakLamp fLamp;
    float    fGainIn, fGainOut, fPeakLin;
    int      fPeakHold;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RossVUPlugin)
};

Plugin* createPlugin() { return new RossVUPlugin(); }

END_NAMESPACE_DISTRHO
