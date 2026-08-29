#include "sat_prism.h"
#include <math.h>

// Helper for inflate_box
static inline float vec3_magnitude(Vec3 v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

Vertices inflate_box(Vertices box, float margin) {
    Vec3 center = {0, 0, 0};
    for (int i = 0; i < 8; i++) {
        center.x += box.vec[i].x;
        center.y += box.vec[i].y;
        center.z += box.vec[i].z;
    }
    center.x /= 8.0f;
    center.y /= 8.0f;
    center.z /= 8.0f;
    
    Vec3 axis_x = vec3_sub(box.vec[1], box.vec[0]);
    Vec3 axis_y = vec3_sub(box.vec[3], box.vec[0]);
    Vec3 axis_z = vec3_sub(box.vec[4], box.vec[0]);
    
    float len_x = vec3_magnitude(axis_x);
    float len_y = vec3_magnitude(axis_y);
    float len_z = vec3_magnitude(axis_z);
    
    if (len_x > 1e-9f) {
        axis_x.x /= len_x; axis_x.y /= len_x; axis_x.z /= len_x;
    }
    if (len_y > 1e-9f) {
        axis_y.x /= len_y; axis_y.y /= len_y; axis_y.z /= len_y;
    }
    if (len_z > 1e-9f) {
        axis_z.x /= len_z; axis_z.y /= len_z; axis_z.z /= len_z;
    }
    
    float half_x = len_x * 0.5f + margin;
    float half_y = len_y * 0.5f + margin;
    float half_z = len_z * 0.5f + margin;
    
    Vec3 hx = vec3_mul(axis_x, half_x);
    Vec3 hy = vec3_mul(axis_y, half_y);
    Vec3 hz = vec3_mul(axis_z, half_z);
    
    Vertices out = {0};
    out.vec[0] = vec3_sub(vec3_sub(vec3_sub(center, hx), hy), hz);
    out.vec[1] = vec3_sub(vec3_sub(vec3_add(center, hx), hy), hz);
    out.vec[2] = vec3_sub(vec3_add(vec3_add(center, hx), hy), hz);
    out.vec[3] = vec3_sub(vec3_add(vec3_sub(center, hx), hy), hz);
    out.vec[4] = vec3_add(vec3_sub(vec3_sub(center, hx), hy), hz);
    out.vec[5] = vec3_add(vec3_sub(vec3_add(center, hx), hy), hz);
    out.vec[6] = vec3_add(vec3_add(vec3_add(center, hx), hy), hz);
    out.vec[7] = vec3_add(vec3_add(vec3_sub(center, hx), hy), hz);
    
    return out;
}

void Project(const Vertices vertices, int count, Vec3 axis, float* minProj, float* maxProj) {
    float minP = vec3_dot(vertices.vec[0], axis);
    float maxP = minP;
    for (int i = 1; i < count; i++) {
        float p = vec3_dot(vertices.vec[i], axis);
        if (p < minP) minP = p;
        else if (p > maxP) maxP = p;
    }
    *minProj = minP;
    *maxProj = maxP;
}

bool PrismsOverlap(const Vertices prism1, const Vertices prism2) {
    Vec3 axes[15];
    int axisCount = 0;
    
    axes[axisCount++] = vec3_sub(prism1.vec[1], prism1.vec[0]);
    axes[axisCount++] = vec3_sub(prism1.vec[3], prism1.vec[0]);
    axes[axisCount++] = vec3_sub(prism1.vec[4], prism1.vec[0]);
    axes[axisCount++] = vec3_sub(prism2.vec[1], prism2.vec[0]);
    axes[axisCount++] = vec3_sub(prism2.vec[3], prism2.vec[0]);
    axes[axisCount++] = vec3_sub(prism2.vec[4], prism2.vec[0]);
    
    for (int i = 0; i < 3; i++) {
        for (int j = 3; j < 6; j++) {
            axes[axisCount++] = vec3_cross(axes[i], axes[j]);
        }
    }
    
    for (int i = 0; i < axisCount; i++) {
        float min1, max1, min2, max2;
        Project(prism1, 8, axes[i], &min1, &max1);
        Project(prism2, 8, axes[i], &min2, &max2);
        if (!IntervalsOverlap(min1, max1, min2, max2))
            return false;
    }
    return true;
}