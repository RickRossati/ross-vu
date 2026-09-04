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