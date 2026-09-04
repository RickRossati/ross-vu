/*
 * ROSS VU // ANALOG ENERGY METER  -- interface
 *
 * Geometria fisica correta: o mostrador e desenhado a partir da deflexao linear
 * do movimento (p = 10^(VU/20) / 10^(3/20)), que e o que produz a escala apertada
 * na esquerda e aberta perto do zero, igual a um VU de verdade.
 */
#include "DistrhoUI.hpp"
#include "RossVUFonts.h"
#include "RossVUPresets.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

START_NAMESPACE_DISTRHO

using DGL_NAMESPACE::Color;

// ------------------------------------------------------------------ geometria
static const float kW = 1000.0f;
static const float kH = 640.0f;

static const float kPivX = 500.0f;
static const float kPivY = 440.0f;

static const float kRBase   = 282.0f;   // curva de base sob os tracos
static const float kRMinor  = 300.0f;
static const float kRMajor  = 312.0f;
static const float kRNum    = 330.0f;
static const float kRNeedle = 324.0f;

static const float kSpan    = 57.0f;    // graus para cada lado do vertical
static const float kFullDef = 1.412538f;// deflexao em +3 VU

static const float kFaceX = 68.0f,  kFaceY = 94.0f;
static const float kFaceW = 864.0f, kFaceH = 356.0f;

static const float kPI = 3.14159265358979f;

// alvos de clique que nao correspondem a um parametro
enum { kHitPresetPrev = 100, kHitPresetNext = 101 };

// seletor de preset no cabecalho
static const float kPresetX = 500.0f, kPresetY = 60.0f;
static const float kArrowL = 404.0f, kArrowR = 596.0f;

// centros das celulas do painel de controle
static const float kCxCal = 151.0f, kCxResp = 336.0f, kCxPeak = 498.0f;
static const float kCxIn  = 664.0f, kCxOut  = 830.0f;
static const float kRowLabel = 500.0f, kRowWidget = 556.0f, kRowSub = 612.0f;

static const float kCalDb[7] = { -20.0f, -18.0f, -16.0f, -14.0f, -12.0f, -10.0f, -8.0f };
static const char* kPeakRefLabel[6] = { "-12 dBTP","-9 dBTP","-6 dBTP","-3 dBTP","-1 dBTP","-0.1 dBTP" };
static const char* kModeName[5] = { "L+R", "LEFT", "RIGHT", "SIDE", "L | R" };

static inline float defToAngle(float def)
{
    float p = def / kFullDef;
    if (p < 0.0f) p = 0.0f;
    if (p > 1.06f) p = 1.06f;
    return (-kSpan + 2.0f * kSpan * p) * kPI / 180.0f;
}

static inline float vuToAngle(float vu)
{
    return defToAngle(std::pow(10.0f, vu * 0.05f));
}

// ------------------------------------------------------------------------- UI
class RossVUUI final : public UI
{
public:
    RossVUUI()
        : UI(static_cast<uint>(kW), static_cast<uint>(kH)),
          fInput(0.0f), fOutput(0.0f), fResponse(0.5f), fCal(1.0f), fMode(0.0f), fPeakRef(2.0f),
          fNeedle(0.0f), fNeedle2(0.0f), fVu(-40.0f), fVu2(-40.0f),
          fPeakDb(-90.0f), fLed(0.0f),
          fDrag(-1), fHover(-1), fPreset(0),
          fDragStartY(0.0), fDragStartVal(0.0f),
          fLastClickTime(0), fLastClickTarget(-1)
    {
        loadFonts();
        setGeometryConstraints(600, 384, true, false);
    }

protected:
    // ------------------------------------------------------------ parametros
    void parameterChanged(uint32_t index, float value) override
    {
        switch (index)
        {
        case kParamInput:       fInput    = value; break;
        case kParamOutput:      fOutput   = value; break;
        case kParamResponse:    fResponse = value; break;
        case kParamCalibration: fCal      = value; break;
        case kParamMode:        fMode     = value; break;
        case kParamPeakRef:     fPeakRef  = value; break;
        case kOutDeflection:    fNeedle   = value; break;
        case kOutDeflection2:   fNeedle2  = value; break;
        case kOutVu:            fVu       = value; break;
        case kOutVu2:           fVu2      = value; break;
        case kOutTruePeak:      fPeakDb   = value; break;
        case kOutLed:           fLed      = value; break;
        default: return;
        }
        repaint();
    }

    // ----------------------------------------------------------------- mouse
    bool onMouse(const MouseEvent& ev) override
    {
        if (ev.button != 1)
            return false;

        const double sx = static_cast<double>(getWidth())  / kW;
        const double sy = static_cast<double>(getHeight()) / kH;
        const float  x  = static_cast<float>(ev.pos.getX() / sx);
        const float  y  = static_cast<float>(ev.pos.getY() / sy);

        if (! ev.press)
        {
            if (fDrag >= 0)
            {
                editParameter(static_cast<uint32_t>(fDrag), false);
                fDrag = -1;
            }
            return true;
        }

        const int target = hitTest(x, y);
        if (target < 0)
            return false;

        // duplo clique volta ao padrao
        const bool dbl = (target == fLastClickTarget)
                      && (ev.time - fLastClickTime < 400);
        fLastClickTime   = ev.time;
        fLastClickTarget = target;

        if (target == kParamCalibration)
        {
            fCal = static_cast<float>((static_cast<int>(fCal + 0.5f) + 1) % 7);
            sendParam(kParamCalibration, fCal);
            repaint();
            return true;
        }

        if (target == kHitPresetPrev || target == kHitPresetNext)
        {
            int i = matchPreset();
            if (i < 0) i = fPreset;
            i += (target == kHitPresetNext) ? 1 : -1;
            if (i < 0) i = kRossVUPresetCount - 1;
            if (i >= kRossVUPresetCount) i = 0;
            applyPreset(i);
            repaint();
            return true;
        }

        if (target == kParamPeakRef)
        {
            fPeakRef = static_cast<float>((static_cast<int>(fPeakRef + 0.5f) + 1) % 6);
            sendParam(kParamPeakRef, fPeakRef);
            repaint();
            return true;
        }

        if (target == kParamMode)
        {
            fMode = static_cast<float>((static_cast<int>(fMode + 0.5f) + 1) % 5);
            sendParam(kParamMode, fMode);
            repaint();
            return true;
        }

        if (dbl)
        {
            const float def = (target == kParamResponse) ? 0.5f : 0.0f;
            setKnob(target, def);
            repaint();
            return true;
        }

        fDrag         = target;
        fDragStartY   = ev.pos.getY();
        fDragStartVal = knobValue(target);
        editParameter(static_cast<uint32_t>(target), true);
        return true;
    }

    bool onMotion(const MotionEvent& ev) override
    {
        if (fDrag < 0)
        {
            // sem isso nada na interface avisa que e clicavel
            const double hx = static_cast<double>(getWidth())  / kW;
            const double hy = static_cast<double>(getHeight()) / kH;
            const int    h  = hitTest(static_cast<float>(ev.pos.getX() / hx),
                                      static_cast<float>(ev.pos.getY() / hy));
            if (h != fHover) { fHover = h; repaint(); }
            return false;
        }

        const double sy    = static_cast<double>(getHeight()) / kH;
        const float  dy    = static_cast<float>((fDragStartY - ev.pos.getY()) / sy);
        const bool   fine  = (ev.mod & kModifierShift) != 0;
        const float  range = knobRange(fDrag);

        float v = fDragStartVal + (dy / 220.0f) * range * (fine ? 0.2f : 1.0f);
        setKnob(fDrag, v);
        repaint();
        return true;
    }

    bool onScroll(const ScrollEvent& ev) override
    {
        const double sx = static_cast<double>(getWidth())  / kW;
        const double sy = static_cast<double>(getHeight()) / kH;
        const int target = hitTest(static_cast<float>(ev.pos.getX() / sx),
                                   static_cast<float>(ev.pos.getY() / sy));

        if (target != kParamInput && target != kParamOutput && target != kParamResponse)
            return false;

        const float step = (target == kParamResponse) ? 0.02f : 0.5f;
        const float d    = ev.delta.getY() > 0.0 ? step : -step;

        editParameter(static_cast<uint32_t>(target), true);
        setKnob(target, knobValue(target) + d);
        editParameter(static_cast<uint32_t>(target), false);
        repaint();
        return true;
    }

    // ---------------------------------------------------------------- desenho
    void onNanoDisplay() override
    {
        save();
        scale(static_cast<float>(getWidth()) / kW, static_cast<float>(getHeight()) / kH);

        drawChassis();
        drawHeader();
        drawBezel();
        drawFace();
        drawScale();
        drawNeedle();
        drawHub();
        drawFaceReadouts();
        drawGlass();
        drawControls();

        restore();
    }

private:
    // ------------------------------------------------------------- utilidades
    void loadFonts()
    {
        // embutidas: sem isso o plugin fica sem texto nenhum no Windows,
        // e no Linux quebraria em qualquer distro sem a Liberation instalada
        createFontFromMemory("ui",  kFontRegular, kFontRegular_size, false);
        createFontFromMemory("uib", kFontBold,    kFontBold_size,    false);
    }

    void tx(float x, float y, int align, float size, const Color& c,
            const char* s, bool bold = false, float spacing = 0.0f)
    {
        fontFace(bold ? "uib" : "ui");
        fontSize(size);
        textLetterSpacing(spacing);
        textAlign(align);
        fillColor(c);
        beginPath();
        text(x, y, s, nullptr);
        textLetterSpacing(0.0f);
    }

    void sendParam(int index, float value)
    {
        editParameter(static_cast<uint32_t>(index), true);
        setParameterValue(static_cast<uint32_t>(index), value);
        editParameter(static_cast<uint32_t>(index), false);
    }

    void applyPreset(int i)
    {
        const RossVUPreset& k = kRossVUPresets[i];
        fCal = k.cal; fResponse = k.response; fMode = k.mode; fPeakRef = k.peak;
        sendParam(kParamCalibration, fCal);
        sendParam(kParamResponse,    fResponse);
        sendParam(kParamMode,        fMode);
        sendParam(kParamPeakRef,     fPeakRef);
        fPreset = i;
    }

    // -1 quando os controles nao batem com nenhum preset
    int matchPreset() const
    {
        for (int i = 0; i < kRossVUPresetCount; ++i)
        {
            const RossVUPreset& k = kRossVUPresets[i];
            if (std::fabs(fCal      - k.cal)  < 0.25f &&
                std::fabs(fMode     - k.mode) < 0.25f &&
                std::fabs(fPeakRef  - k.peak) < 0.25f &&
                std::fabs(fResponse - k.response) < 0.005f)
                return i;
        }
        return -1;
    }

    float knobValue(int index) const
    {
        if (index == kParamInput)    return fInput;
        if (index == kParamOutput)   return fOutput;
        if (index == kParamResponse) return fResponse;
        return 0.0f;
    }

    static float knobRange(int index)
    {
        return (index == kParamResponse) ? 1.0f : 48.0f;
    }

    void setKnob(int index, float v)
    {
        if (index == kParamResponse)
        {
            v = std::fmin(std::fmax(v, 0.0f), 1.0f);
            fResponse = v;
        }
        else
        {
            v = std::fmin(std::fmax(v, -24.0f), 24.0f);
            if (index == kParamInput) fInput = v; else fOutput = v;
        }
        setParameterValue(static_cast<uint32_t>(index), v);
    }

    int hitTest(float x, float y) const
    {
        if (near(x, y, kCxResp, kRowWidget, 42.0f)) return kParamResponse;
        if (near(x, y, kCxIn,   kRowWidget, 42.0f)) return kParamInput;
        if (near(x, y, kCxOut,  kRowWidget, 42.0f)) return kParamOutput;
        if (near(x, y, kCxCal,  kRowWidget, 26.0f)) return kParamCalibration;
        if (near(x, y, kCxPeak, kRowWidget, 26.0f)) return kParamPeakRef;
        if (near(x, y, kArrowL, kPresetY, 20.0f))   return kHitPresetPrev;
        if (near(x, y, kArrowR, kPresetY, 20.0f))   return kHitPresetNext;
        if (x > 86.0f && x < 178.0f && y > 402.0f && y < 430.0f) return kParamMode;
        return -1;
    }

    static bool near(float x, float y, float cx, float cy, float r)
    {
        const float dx = x - cx, dy = y - cy;
        return dx * dx + dy * dy <= r * r;
    }

    // --------------------------------------------------------------- chassi
    void drawChassis()
    {
        // sem margem e sem contorno claro: a moldura de 6 px que existia aqui
        // deixava aparecer o grao escovado e virava uma bordinha feia
        beginPath(); rect(0, 0, kW, kH);
        fillPaint(linearGradient(0, 0, 0, kH, Color(28, 28, 32), Color(12, 12, 14)));
        fill();

        for (float x = 0.0f; x < kW; x += 3.0f)
        {
            const int n = static_cast<int>(x) % 17;
            beginPath(); rect(x, 0, 1.4f, kH);
            fillColor(Color(255, 250, 240, 0.014f + (n % 5) * 0.004f)); fill();
        }

        beginPath(); roundedRect(20, 18, kW - 40, kH - 36, 6);
        fillColor(Color(15, 15, 17)); fill();
        strokeColor(Color(4, 4, 5)); strokeWidth(1); stroke();
    }

    void drawHeader()
    {
        tx(62, 60, ALIGN_LEFT | ALIGN_MIDDLE, 30, Color(233, 221, 194),
           "ROSS VU", true, 5.0f);
        tx(938, 60, ALIGN_RIGHT | ALIGN_MIDDLE, 13, Color(139, 133, 119),
           "ANALOG ENERGY METER", false, 4.5f);

        drawPresetSelector();
    }

    void drawPresetSelector()
    {
        const int  i    = matchPreset();
        const bool over = fHover == kHitPresetPrev || fHover == kHitPresetNext;

        // trilho, para o conjunto ler como um controle e nao como texto solto
        beginPath(); roundedRect(kArrowL - 20, kPresetY - 17, (kArrowR + 20) - (kArrowL - 20), 34, 3);
        fillColor(Color(0, 0, 0, 0.275f)); fill();
        strokeColor(over ? Color(96, 90, 76) : Color(56, 54, 50));
        strokeWidth(1); stroke();

        tx(kPresetX, kPresetY - 1, ALIGN_CENTER | ALIGN_MIDDLE, 13,
           i >= 0 ? Color(214, 196, 150) : Color(128, 122, 110),
           i >= 0 ? kRossVUPresets[i].name : "CUSTOM", true, 3.0f);

        drawArrow(kArrowL, kPresetY, -1.0f, fHover == kHitPresetPrev);
        drawArrow(kArrowR, kPresetY, +1.0f, fHover == kHitPresetNext);
    }

    void drawArrow(float cx, float cy, float dir, bool over)
    {
        const Color c = over ? Color(232, 216, 178) : Color(126, 120, 108);
        beginPath();
        moveTo(cx - dir * 3.5f, cy - 6.0f);
        lineTo(cx + dir * 3.5f, cy);
        lineTo(cx - dir * 3.5f, cy + 6.0f);
        strokeColor(c); strokeWidth(2.0f); lineCap(ROUND); lineJoin(ROUND); stroke();
    }

    // ---------------------------------------------------------------- moldura
    void drawBezel()
    {
        beginPath(); roundedRect(50, 76, 900, 392, 14);
        fillPaint(linearGradient(0, 76, 0, 468, Color(44, 44, 49), Color(9, 9, 11)));
        fill();

        beginPath(); roundedRect(50.5f, 76.5f, 899, 391, 14);
        strokeColor(Color(70, 70, 78)); strokeWidth(1); stroke();

        beginPath(); roundedRect(62, 88, 876, 368, 8);
        fillColor(Color(6, 6, 7)); fill();

        beginPath(); roundedRect(62.5f, 88.5f, 875, 367, 8);
        strokeColor(Color(0, 0, 0)); strokeWidth(1); stroke();
    }

    void drawFace()
    {
        beginPath(); roundedRect(kFaceX, kFaceY, kFaceW, kFaceH, 4);
        fillPaint(radialGradient(kPivX, kPivY - 30.0f, 30.0f, 540.0f,
                                 Color(246, 231, 189), Color(196, 170, 118)));
        fill();

        // lampada por tras do mostrador
        beginPath(); roundedRect(kFaceX, kFaceY, kFaceW, kFaceH, 4);
        fillPaint(radialGradient(kPivX, kPivY - 60.0f, 20.0f, 300.0f,
                                 Color(255, 246, 214, 0.353f), Color(255, 246, 214, 0.000f)));
        fill();

        // sombra interna nas bordas
        beginPath(); roundedRect(kFaceX, kFaceY, kFaceW, kFaceH, 4);
        fillPaint(boxGradient(kFaceX, kFaceY, kFaceW, kFaceH, 6, 78,
                              Color(0, 0, 0, 0.000f), Color(66, 46, 18, 0.62f)));
        fill();
    }

    // ----------------------------------------------------------------- escala
    void drawScale()
    {
        const Color ink(28, 23, 17);
        const Color red(172, 36, 40);

        // curva de base
        arcStroke(kRBase, vuToAngle(-24.0f), vuToAngle(0.0f), ink, 2.4f);
        arcStroke(kRBase + 7.0f, vuToAngle(0.0f), vuToAngle(3.0f), red, 5.0f);

        // degrau vermelho no zero
        radial(vuToAngle(0.0f), kRBase - 1.0f, kRBase + 9.5f, red, 5.0f);
        radial(vuToAngle(3.0f), kRBase + 4.0f, kRBase + 10.0f, red, 5.0f);

        static const float majors[] = { -20,-10,-7,-5,-3,-2,-1,0,1,2,3 };
        static const char* labels[] = { "-20","-10","-7","-5","-3","-2","-1","0","+1","+2","+3" };
        static const float minors[] = { -15,-12,-9,-8,-6,-4,-3.5f,-2.5f,-1.5f,-0.5f,0.5f,1.5f,2.5f };

        for (int i = 0; i < 13; ++i)
        {
            const float v = minors[i];
            radial(vuToAngle(v), kRBase, kRMinor, v >= 0.0f ? red : ink, v >= 0.0f ? 2.6f : 1.8f);
        }

        for (int i = 0; i < 11; ++i)
        {
            const float v = majors[i];
            const bool  hot = v >= 0.0f;
            radial(vuToAngle(v), kRBase, kRMajor, hot ? red : ink, hot ? 4.2f : 3.2f);

            const float a = vuToAngle(v);
            const float nx = kPivX + kRNum * std::sin(a);
            const float ny = kPivY - kRNum * std::cos(a);
            tx(nx, ny, ALIGN_CENTER | ALIGN_MIDDLE, 27,
               hot ? Color(168, 32, 36) : Color(24, 20, 15), labels[i], false);
        }

        tx(kPivX, 352, ALIGN_CENTER | ALIGN_MIDDLE, 62, Color(24, 20, 15), "VU", true, 2.0f);
    }

    void radial(float a, float r0, float r1, const Color& c, float w)
    {
        const float s = std::sin(a), co = std::cos(a);
        beginPath();
        moveTo(kPivX + r0 * s, kPivY - r0 * co);
        lineTo(kPivX + r1 * s, kPivY - r1 * co);
        strokeColor(c); strokeWidth(w); lineCap(ROUND); stroke();
    }

    void arcStroke(float r, float a0, float a1, const Color& c, float w)
    {
        // angulos aqui sao medidos a partir da vertical; nanovg usa o eixo x
        beginPath();
        arc(kPivX, kPivY, r, a0 - kPI * 0.5f, a1 - kPI * 0.5f, CW);
        strokeColor(c); strokeWidth(w); lineCap(BUTT); stroke();
    }

    // ---------------------------------------------------------------- ponteiro
    void drawNeedle()
    {
        const bool dual = static_cast<int>(fMode + 0.5f) == 4;

        if (dual)
        {
            // ponteiro do canal direito: mais fino e mais claro, atras do principal
            const float b = defToAngle(fNeedle2);
            save();
            translate(kPivX, kPivY);
            rotate(b);
            beginPath();
            moveTo(-2.2f, 14.0f);
            lineTo(-0.7f, -kRNeedle + 8.0f);
            lineTo( 0.7f, -kRNeedle + 8.0f);
            lineTo( 2.2f, 14.0f);
            closePath();
            fillColor(Color(122, 88, 34, 0.824f));
            fill();
            restore();
        }

        const float a = defToAngle(fNeedle);

        // sombra projetada, deslocada na tela e nao no eixo do ponteiro
        save();
        translate(kPivX + 2.5f, kPivY + 5.0f);
        rotate(a);
        needlePath();
        fillColor(Color(96, 72, 32, 0.216f));
        fill();
        restore();

        save();
        translate(kPivX, kPivY);
        rotate(a);

        needlePath();
        fillColor(Color(20, 17, 13));
        fill();

        // contrapeso
        beginPath(); roundedRect(-5.5f, 12, 11, 34, 5);
        fillColor(Color(24, 21, 17)); fill();

        restore();
    }

    void needlePath()
    {
        beginPath();
        moveTo(-3.6f, 14.0f);
        lineTo(-1.0f, -kRNeedle);
        lineTo( 0.0f, -kRNeedle - 4.0f);
        lineTo( 1.0f, -kRNeedle);
        lineTo( 3.6f, 14.0f);
        closePath();
    }

    void drawHub()
    {
        // base preta sob o eixo, como na caixa original
        beginPath();
        moveTo(kPivX - 62, kFaceY + kFaceH);
        lineTo(kPivX - 44, kPivY - 12);
        lineTo(kPivX + 44, kPivY - 12);
        lineTo(kPivX + 62, kFaceY + kFaceH);
        closePath();
        fillColor(Color(18, 16, 14)); fill();

        beginPath(); circle(kPivX, kPivY, 36);
        fillPaint(linearGradient(kPivX, kPivY - 36, kPivX, kPivY + 36,
                                 Color(126, 128, 132), Color(28, 29, 32)));
        fill();

        beginPath(); circle(kPivX, kPivY, 36);
        strokeColor(Color(10, 10, 12)); strokeWidth(1.5f); stroke();

        beginPath(); circle(kPivX, kPivY, 22);
        fillPaint(linearGradient(kPivX, kPivY - 22, kPivX, kPivY + 22,
                                 Color(46, 47, 50), Color(96, 98, 102)));
        fill();

        beginPath(); circle(kPivX, kPivY, 9);
        fillColor(Color(16, 16, 18)); fill();
        beginPath(); circle(kPivX - 2.5f, kPivY - 3, 3);
        fillColor(Color(120, 122, 126, 0.627f)); fill();
    }

    void drawFaceReadouts()
    {
        char buf[64];
        const int  mode = std::min(std::max(static_cast<int>(fMode + 0.5f), 0), 4);
        const bool dual = mode == 4;

        // moldura fina: sem ela ninguem descobre que da para clicar e trocar de modo
        {
            const bool over = fHover == kParamMode;
            beginPath(); roundedRect(86, 402, 92, 28, 3);
            fillColor(Color(90, 70, 34, over ? 0.133f : 0.063f)); fill();
            strokeColor(Color(90, 70, 34, over ? 0.588f : 0.306f));
            strokeWidth(1); stroke();
        }
        tx(98, 416, ALIGN_LEFT | ALIGN_MIDDLE, 15,
           fHover == kParamMode ? Color(88, 68, 30) : Color(120, 98, 58),
           kModeName[mode], true, 2.0f);

        if (dual)
        {
            if (fVu > -39.0f) std::snprintf(buf, sizeof(buf), "L %+.1f   R %+.1f", fVu, fVu2);
            else              std::snprintf(buf, sizeof(buf), "L --   R --");
        }
        else
        {
            if (fVu > -39.0f) std::snprintf(buf, sizeof(buf), "%+.1f VU", fVu);
            else              std::snprintf(buf, sizeof(buf), "-- VU");
        }
        tx(96, 438, ALIGN_LEFT | ALIGN_MIDDLE, 15, Color(120, 98, 58), buf, false, 1.0f);

        if (fPeakDb > -89.0f)
            std::snprintf(buf, sizeof(buf), "TRUE PEAK %+.1f dBTP", fPeakDb);
        else
            std::snprintf(buf, sizeof(buf), "TRUE PEAK  --");
        tx(904, 438, ALIGN_RIGHT | ALIGN_MIDDLE, 15,
           fPeakDb > -0.1f ? Color(168, 32, 36) : Color(120, 98, 58), buf, false, 1.0f);
    }

    void drawGlass()
    {
        // brilho diagonal do vidro no canto superior esquerdo
        beginPath();
        moveTo(kFaceX, kFaceY);
        lineTo(kFaceX + 300, kFaceY);
        lineTo(kFaceX, kFaceY + 210);
        closePath();
        fillPaint(linearGradient(kFaceX, kFaceY, kFaceX + 98.4f, kFaceY + 141.1f,
                                 Color(255, 252, 244, 0.105f), Color(255, 252, 244, 0.000f)));
        fill();

        // reflexo fino junto da borda de cima
        beginPath(); rect(kFaceX, kFaceY, kFaceW, 26);
        fillPaint(linearGradient(kFaceX, kFaceY, kFaceX, kFaceY + 26,
                                 Color(255, 255, 255, 0.102f), Color(255, 255, 255, 0.000f)));
        fill();

        beginPath(); roundedRect(kFaceX, kFaceY, kFaceW, kFaceH, 4);
        strokeColor(Color(0, 0, 0, 0.353f)); strokeWidth(1.5f); stroke();
    }

    // --------------------------------------------------------------- controles
    void drawControls()
    {
        static const float div[4] = { 236, 423, 572, 739 };
        for (int i = 0; i < 4; ++i)
        {
            beginPath();
            moveTo(div[i], 486); lineTo(div[i], 620);
            strokeColor(Color(52, 52, 58)); strokeWidth(1); stroke();
        }

        char buf[48];

        // calibracao
        const int ci = std::min(std::max(static_cast<int>(fCal + 0.5f), 0), 6);
        std::snprintf(buf, sizeof(buf), "CAL %d", static_cast<int>(kCalDb[ci]));
        tx(kCxCal, kRowLabel, ALIGN_CENTER | ALIGN_MIDDLE, 15, Color(203, 193, 172), buf, false, 3.0f);
        drawCalButton(kCxCal, kRowWidget);
        tx(kCxCal, kRowSub, ALIGN_CENTER | ALIGN_MIDDLE, 11, Color(120, 115, 104),
           "0 VU REF", false, 2.0f);

        // resposta
        tx(kCxResp, kRowLabel, ALIGN_CENTER | ALIGN_MIDDLE, 15, Color(203, 193, 172),
           "RESPONSE", false, 3.0f);
        drawKnob(kCxResp, kRowWidget, fResponse, kParamResponse);
        std::snprintf(buf, sizeof(buf), "%d ms", static_cast<int>(responseMs() + 0.5f));
        tx(kCxResp, kRowSub - 2, ALIGN_CENTER | ALIGN_MIDDLE, 12, Color(160, 152, 136), buf);
        tx(272, kRowSub - 2, ALIGN_LEFT | ALIGN_MIDDLE, 11, Color(112, 107, 97), "SLOW", false, 1.5f);
        tx(400, kRowSub - 2, ALIGN_RIGHT | ALIGN_MIDDLE, 11, Color(112, 107, 97), "FAST", false, 1.5f);

        // pico
        tx(kCxPeak, kRowLabel, ALIGN_CENTER | ALIGN_MIDDLE, 15, Color(203, 193, 172),
           "PEAK", false, 3.0f);
        drawLed(kCxPeak, kRowWidget, fLed > 0.5f);
        {
            const int pi = std::min(std::max(static_cast<int>(fPeakRef + 0.5f), 0), 5);
            tx(kCxPeak, kRowSub, ALIGN_CENTER | ALIGN_MIDDLE, 11, Color(160, 152, 136),
               kPeakRefLabel[pi], false, 1.5f);
        }

        // entrada e saida
        tx(kCxIn, kRowLabel, ALIGN_CENTER | ALIGN_MIDDLE, 15, Color(203, 193, 172),
           "INPUT", false, 3.0f);
        drawKnob(kCxIn, kRowWidget, (fInput + 24.0f) / 48.0f, kParamInput);
        std::snprintf(buf, sizeof(buf), "%+.1f dB", fInput);
        tx(kCxIn, kRowSub - 2, ALIGN_CENTER | ALIGN_MIDDLE, 12, Color(160, 152, 136), buf);

        tx(kCxOut, kRowLabel, ALIGN_CENTER | ALIGN_MIDDLE, 15, Color(203, 193, 172),
           "OUTPUT", false, 3.0f);
        drawKnob(kCxOut, kRowWidget, (fOutput + 24.0f) / 48.0f, kParamOutput);
        std::snprintf(buf, sizeof(buf), "%+.1f dB", fOutput);
        tx(kCxOut, kRowSub - 2, ALIGN_CENTER | ALIGN_MIDDLE, 12, Color(160, 152, 136), buf);
    }

    float responseMs() const
    {
        const float k = std::fmin(std::fmax(fResponse, 0.0f), 1.0f);
        const float t = (k <= 0.5f) ? (0.600f - 0.600f * k)
                                    : (0.300f - 0.400f * (k - 0.5f));
        return t * 1000.0f;
    }

    void drawKnob(float cx, float cy, float norm, int id)
    {
        const bool over = (fHover == id) || (fDrag == id);
        norm = std::fmin(std::fmax(norm, 0.0f), 1.0f);

        // anel de pontos
        for (int i = 0; i < 11; ++i)
        {
            const float a = (-140.0f + 280.0f * (i / 10.0f)) * kPI / 180.0f;
            beginPath();
            circle(cx + 46.0f * std::sin(a), cy - 46.0f * std::cos(a), 2.0f);
            fillColor(i == 5 ? Color(214, 196, 150) : Color(150, 140, 118)); fill();
        }

        beginPath(); circle(cx, cy + 3, 33); fillColor(Color(0, 0, 0, 0.471f)); fill();

        beginPath(); circle(cx, cy, 32);
        fillPaint(linearGradient(cx, cy - 32, cx, cy + 32, Color(58, 58, 62), Color(14, 14, 16)));
        fill();

        // recartilhado
        for (int i = 0; i < 40; ++i)
        {
            const float a = i * (2.0f * kPI / 40.0f);
            beginPath();
            moveTo(cx + 26.0f * std::sin(a), cy - 26.0f * std::cos(a));
            lineTo(cx + 31.5f * std::sin(a), cy - 31.5f * std::cos(a));
            strokeColor(Color(255, 255, 255, i % 2 ? 0.10f : 0.24f)); strokeWidth(1.2f); stroke();
        }

        beginPath(); circle(cx, cy, 24);
        fillPaint(linearGradient(cx, cy - 24, cx, cy + 24, Color(54, 54, 58), Color(18, 18, 21)));
        fill();
        beginPath(); circle(cx, cy, 24);
        strokeColor(Color(255, 255, 255, 0.09f)); strokeWidth(1); stroke();

        beginPath(); circle(cx, cy, 32);
        strokeColor(over ? Color(190, 172, 132) : Color(78, 78, 84));
        strokeWidth(over ? 1.5f : 1.0f); stroke();

        const float a = (-140.0f + 280.0f * norm) * kPI / 180.0f;
        beginPath();
        moveTo(cx + 6.0f * std::sin(a), cy - 6.0f * std::cos(a));
        lineTo(cx + 27.0f * std::sin(a), cy - 27.0f * std::cos(a));
        strokeColor(Color(238, 234, 224)); strokeWidth(3); lineCap(ROUND); stroke();
    }

    void drawCalButton(float cx, float cy)
    {
        const bool over = fHover == kParamCalibration;

        beginPath(); circle(cx, cy + 2, 15); fillColor(Color(0, 0, 0, 0.471f)); fill();

        beginPath(); circle(cx, cy, 14);
        fillPaint(linearGradient(cx, cy - 14, cx, cy + 14,
                                 over ? Color(70, 68, 62) : Color(52, 52, 56),
                                 Color(15, 15, 17)));
        fill();
        beginPath(); circle(cx, cy, 14);
        strokeColor(over ? Color(196, 176, 132) : Color(74, 74, 80));
        strokeWidth(over ? 1.4f : 1.0f); stroke();

        beginPath(); circle(cx, cy - 6, 2.4f);
        fillColor(over ? Color(248, 226, 168) : Color(226, 198, 128)); fill();
    }

    void drawLed(float cx, float cy, bool on)
    {
        if (on)
        {
            beginPath(); circle(cx, cy, 30);
            fillPaint(radialGradient(cx, cy, 4, 30, Color(255, 40, 30, 0.510f), Color(255, 40, 30, 0.000f)));
            fill();
        }

        beginPath(); circle(cx, cy, 16);
        fillPaint(linearGradient(cx, cy - 16, cx, cy + 16, Color(40, 40, 44), Color(12, 12, 14)));
        fill();
        beginPath(); circle(cx, cy, 16);
        strokeColor(fHover == kParamPeakRef ? Color(196, 176, 132) : Color(66, 66, 72));
        strokeWidth(fHover == kParamPeakRef ? 1.4f : 1.0f); stroke();

        beginPath(); circle(cx, cy, 10);
        fillPaint(radialGradient(cx - 3, cy - 4, 1, 13,
                                 on ? Color(255, 168, 150) : Color(96, 26, 24),
                                 on ? Color(206, 16, 10)   : Color(44, 10, 10)));
        fill();

        beginPath(); circle(cx - 3.5f, cy - 4.5f, 2.6f);
        fillColor(Color(255, 255, 255, on ? 0.706f : 0.235f)); fill();
    }

    float fInput, fOutput, fResponse, fCal, fMode, fPeakRef;
    float fNeedle, fNeedle2, fVu, fVu2, fPeakDb, fLed;

    int    fDrag, fHover, fPreset;
    double fDragStartY;
    float  fDragStartVal;
    uint   fLastClickTime;
    int    fLastClickTarget;

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RossVUUI)
};

UI* createUI() { return new RossVUUI(); }

END_NAMESPACE_DISTRHO
