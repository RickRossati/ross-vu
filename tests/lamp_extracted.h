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