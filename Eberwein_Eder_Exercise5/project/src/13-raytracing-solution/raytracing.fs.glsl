#version 330 core
precision mediump float;

in vec3 upOffset;
in vec3 rightOffset;
in vec3 rayOrigin;
in vec3 rayDir;

out vec4 fragColor;

uniform vec3 lightPosition;
uniform int maxDepth;
uniform vec2 viewportSize;

#define INFINITY 100000.0
#define EPSILON 1e-5
#define RAY_OFFSET 0.0001

// ------------------------------------------------------------------------
// GRID FLOOR
// ------------------------------------------------------------------------
// Intersect the plane y = 0 and compute a black & white grid pattern.
// The grid is defined with a given cell size and line thickness.
float rayGridFloorIntersection(vec3 ro, vec3 rd, out vec3 floorColor) {
    if (abs(rd.y) < EPSILON) return INFINITY;
    float t = -ro.y / rd.y;
    if(t < EPSILON) return INFINITY;
    vec3 hitPos = ro + t * rd;
    
    float cellSize = 1.0;
    float lineWidth = 0.05;
    // Use mod() so that the grid stays fixed in world space.
    vec2 modPos = mod(hitPos.xz, cellSize);
    float distToLine = min(modPos.x, cellSize - modPos.x);
    distToLine = min(distToLine, min(modPos.y, cellSize - modPos.y));
    
    // If close to a grid line, color is black; otherwise white.
    if(distToLine < lineWidth)
        floorColor = vec3(0.0);
    else
        floorColor = vec3(1.0);
    return t;
}

// ------------------------------------------------------------------------
// TORUS SDF FUNCTIONS & SPHERE TRACING INTERSECTION
// ------------------------------------------------------------------------
// The signed distance function (SDF) for a torus centered at the origin,
// oriented around the y‑axis. The torus is defined by (R, r) where R is the
// major (ring) radius and r is the minor (tube) radius.
float sdTorus(vec3 p, vec2 t) {
    // t.x = major radius, t.y = minor radius
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}

// Using sphere tracing (ray marching) to find an intersection with a torus.
// ro: ray origin, rd: normalized ray direction.
// torusCenter: world-space center of the torus.
// R: major radius, r: minor radius.
float intersectTorus(vec3 ro, vec3 rd, vec3 torusCenter, float R, float r) {
    const int MAX_STEPS = 128;
    const float tMax = 100.0;
    float t = 0.0;
    for (int i = 0; i < MAX_STEPS; i++) {
        vec3 pos = ro + rd * t - torusCenter;
        float d = sdTorus(pos, vec2(R, r));
        if (d < EPSILON)
            return t;
        t += d;
        if (t > tMax) break;
    }
    return INFINITY;
}

// Approximate the normal at the surface of the torus using finite differences.
vec3 getTorusNormal(vec3 pos, vec3 torusCenter, float R, float r) {
    float eps = 0.001;
    vec3 p = pos - torusCenter;
    vec3 grad;
    grad.x = sdTorus(p + vec3(eps, 0.0, 0.0), vec2(R, r)) - sdTorus(p - vec3(eps, 0.0, 0.0), vec2(R, r));
    grad.y = sdTorus(p + vec3(0.0, eps, 0.0), vec2(R, r)) - sdTorus(p - vec3(0.0, eps, 0.0), vec2(R, r));
    grad.z = sdTorus(p + vec3(0.0, 0.0, eps), vec2(R, r)) - sdTorus(p - vec3(0.0, 0.0, eps), vec2(R, r));
    return normalize(grad);
}

// Add a torus to the scene. If the torus is hit closer than any previous hit,
// update the hit distance, color, and normal.
void addTorus(vec3 ro, vec3 rd, vec3 torusCenter, float R, float r, vec3 color,
              inout float hitDist, inout vec3 hitColor, inout vec3 hitNormal) {
    float t = intersectTorus(ro, rd, torusCenter, R, r);
    if(t < hitDist) {
        vec3 pos = ro + rd * t;
        hitNormal = getTorusNormal(pos, torusCenter, R, r);
        hitDist = t;
        hitColor = color;
    }
}

// ------------------------------------------------------------------------
// SCENE DEFINITION
// ------------------------------------------------------------------------
// The scene consists of a grid floor (plane y = 0) and three blue toruses
// stacked on top of each other.
float rayTraceScene(vec3 ro, vec3 rd, out vec3 hitNormal, out vec3 hitColor) {
    float hitDist = INFINITY;
    hitNormal = vec3(0.0);

    // Grid floor
    vec3 floorColor;
    float tFloor = rayGridFloorIntersection(ro, rd, floorColor);
    if(tFloor < hitDist) {
        hitDist = tFloor;
        hitColor = floorColor;
        hitNormal = vec3(0.0, 1.0, 0.0);
    }

    // Three blue toruses stacked vertically.
    // Each torus has major radius 0.7 and minor radius 0.2.
    addTorus(ro, rd, vec3(2.0, 0.5, 2.0), 0.7, 0.2, vec3(0.0, 0.0, 1.0), hitDist, hitColor, hitNormal);
    addTorus(ro, rd, vec3(2.0, 1.5, 2.0), 0.7, 0.2, vec3(0.0, 0.0, 1.0), hitDist, hitColor, hitNormal);
    addTorus(ro, rd, vec3(2.0, 2.5, 2.0), 0.7, 0.2, vec3(0.0, 0.0, 1.0), hitDist, hitColor, hitNormal);

    return hitDist;
}

// ------------------------------------------------------------------------
// LIGHTING FUNCTIONS
// ------------------------------------------------------------------------
// Compute a Fresnel term for the reflection weight.
float calcFresnel(vec3 normal, vec3 inRay) {
    float bias = 0.0;
    float cosTheta = clamp(dot(normal, -inRay), 0.0, 1.0);
    return clamp(bias + pow(1.0 - cosTheta, 2.0), 0.0, 1.0);
}

// Basic Blinn–Phong lighting.
vec3 calcLighting(vec3 hitPoint, vec3 normal, vec3 inRay, vec3 color) {
    vec3 ambient = vec3(0.1);
    vec3 lightVec = lightPosition - hitPoint;
    vec3 lightDir = normalize(lightVec);
    float lightDist = length(lightVec);
    
    // Simple shadow: if something is hit between the hit point and the light,
    // only ambient light is added.
    vec3 shadowNormal, shadowColor;
    float shadowT = rayTraceScene(hitPoint + lightDir * RAY_OFFSET, lightDir, shadowNormal, shadowColor);
    if(shadowT < lightDist)
        return ambient * color;
    else {
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 h = normalize(-inRay + lightDir);
        float ndoth = max(dot(normal, h), 0.0);
        float spec = max(pow(ndoth, 50.0), 0.0);
        return min((ambient + vec3(diff)) * color + vec3(spec), vec3(1.0));
    }
}

// ------------------------------------------------------------------------
// MAIN RAYTRACING LOOP
// ------------------------------------------------------------------------
void main() {
    fragColor = vec4(0.0);
    vec3 ro = rayOrigin;
    vec3 rd = normalize(rayDir);
    
    vec3 hitColor;
    vec3 hitNormal;
    
    float totalWeight = 0.0;
    vec3 accumColor = vec3(0.0);
    
    // Reflection loop (up to maxDepth bounces)
    for (int i = 0; i < maxDepth; i++) {
        float t = rayTraceScene(ro, rd, hitNormal, hitColor);
        if (t >= INFINITY) break;
        
        vec3 hitPoint = ro + t * rd;
        float fresnel = calcFresnel(hitNormal, rd);
        float weight = (1.0 - fresnel) * (1.0 - totalWeight);
        totalWeight += weight;
        accumColor += calcLighting(hitPoint, hitNormal, rd, hitColor) * weight;
        
        // Reflect the ray and offset to avoid self-intersection.
        rd = reflect(rd, hitNormal);
        rd = normalize(rd);
        ro = hitPoint + hitNormal * RAY_OFFSET;
    }
    
    if(totalWeight > 0.0)
        accumColor /= totalWeight;
        
    fragColor = vec4(accumColor, 1.0);
}
