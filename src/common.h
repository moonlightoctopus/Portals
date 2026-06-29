#pragma once
#include <vector>
#include <algorithm>
#include <cmath>

inline int sign(int x) { return x == 0 ? 0 : x < 0 ? -1 : 1; }
inline float sign(float x) { return x == 0.0f ? 0.0f : x < 0.0f ? -1.0f : 1.0f; }

struct Portal {
    float data[4];
    Portal(float x, float y, float z, float a) {
        data[0] = x; data[1] = y; data[2] = z; data[3] = a;
    }
};

struct EngineData {
    float camPos[3];
    float camRot[3];
    float velocity[3];
    int dimension;
    std::vector<Portal> portals;
    float time;
};

extern EngineData gData;

inline void iScene(float ro[3], float rd[3], const std::vector<Portal>& portals, float result[4]) {
    float t = 1e20f;
    bool hit = false;
    int hitId = -1;
    float normal[3] = {0.0f, 0.0f, 0.0f};

    for (size_t i = 0; i < portals.size(); i++) {
        float* p = (float*)portals[i].data;
        float localRo[3] = {ro[0] - p[0], ro[1] - p[1], ro[2] - p[2]};
        
        float tp = -localRo[2] / rd[2];
        if (tp > 0.0f && tp < t) {
            float px = localRo[0] + rd[0] * tp;
            float py = localRo[1] + rd[1] * tp;
            
            if (sqrt(px * px + (py - 0.5f) * (py - 0.5f)) <= 0.7f && py > 0.0f) {
                t = tp;
                hit = true;
                hitId = 0; 
                normal[0] = 0.0f; normal[1] = 0.0f; 
                normal[2] = (float)sign(ro[2] + rd[2] * (t - 0.01f));
            }
        }
    }

    float tf = -ro[1] / rd[1];
    if (tf > 0.0f && tf < t) {
        t = tf;
        hit = true;
        hitId = 1;
        normal[0] = 0.0f; normal[1] = 1.0f; normal[2] = 0.0f;
    }

    result[0] = hit ? 1.0f : 0.0f;
    result[1] = t;
    result[2] = (float)hitId;
    result[3] = normal[1]; 
}

inline void raycast(float ro[3], float rd[3], const std::vector<Portal>& portals, float result[3], int dimension) {
    float tf = -ro[1] / rd[1];
    float td = tf;
    bool hit = tf > 0.0;

    result[0] = hit ? 1.0f : 0.0f;
    result[1] = td;
    result[2] = (float)dimension;
}