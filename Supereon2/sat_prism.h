#ifndef SAT_PRISM_H
#define SAT_PRISM_H

#include <stdbool.h>
#include <stdint.h>

/* =========================
   Vec3 definition
   ========================= */

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    uint8_t R, G, B;
} Color3;

typedef struct {
    Vec3 vec[8];
} Vertices;

/* =========================
   Vec3 math helpers
   ========================= */

static inline Vec3 vec3(float x, float y, float z)
{
    Vec3 v = { x, y, z };
    return v;
}

static inline Vec3 vec3_add(Vec3 a, Vec3 b)
{
    return vec3(a.x + b.x, a.y + b.y, a.z + b.z);
}

static inline Vec3 vec3_sub(Vec3 a, Vec3 b)
{
    return vec3(a.x - b.x, a.y - b.y, a.z - b.z);
}

static inline Vec3 vec3_mul(Vec3 a, float scalar)
{
    return vec3(a.x * scalar, a.y * scalar, a.z * scalar);
}

static inline float vec3_dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

static inline Vec3 vec3_cross(Vec3 a, Vec3 b)
{
    return vec3(
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    );
}

/* =========================
   SAT helpers
   ========================= */

void Project(
    const Vertices vertices,
    int count,
    Vec3 axis,
    float* minProj,
    float* maxProj
);

static inline bool IntervalsOverlap(
    float min1, float max1,
    float min2, float max2
)
{
    return max1 >= min2 && max2 >= min1;
}

/* =========================
   Prism / OBB overlap test
   ========================= */

bool PrismsOverlap(
    const Vertices prism1,
    const Vertices prism2
);

/* =========================
   Inflate box by margin (preserves shape)
   ========================= */

Vertices inflate_box(Vertices box, float margin);

#endif /* SAT_PRISM_H */