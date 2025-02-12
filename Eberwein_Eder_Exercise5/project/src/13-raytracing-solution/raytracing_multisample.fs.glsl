/**
 * A raytracer with multisampling (2×2 grid) for the new scene.
 */
precision mediump float;

in vec3 upOffset;
in vec3 rightOffset;
in vec3 rayOrigin;
in vec3 rayDir;

uniform vec3 lightPosition;
uniform int maxDepth;
uniform vec2 viewportSize;

#define INFINITY 100000.0
#define EPSILON 1e-5
#define RAY_OFFSET 0.0001
#define LIGHT_SIZE 0.1
#define SHADOW_SAMPLES 2

// --- (Repeat the same functions as in raytracing.fs.glsl) ---
// Honeycomb floor functions:
float rayHoneycombFloorIntersection(vec3 ro, vec3 rd, out vec3 floorColor) {
    if (abs(rd.y) < EPSILON) return INFINITY;
    float t = -ro.y / rd.y;
    if(t < EPSILON) return INFINITY;
    vec3 hitPos = ro + t * rd;
    float cellSize = 1.0;
    vec2 p = hitPos.xz / cellSize;
    vec2 q;
    q.x = (sqrt(3.0)/3.0 * p.x - 1.0/3.0 * p.y);
    q.y = (2.0/3.0 * p.y);
    vec2 cell = floor(q + 0.5);
    float index = mod(cell.x + cell.y, 2.0);
    floorColor = mix(vec3(0.8, 0.7, 0.3), vec3(0.3, 0.2, 0.1), step(0.5, index));
    return t;
}

void addHoneycombFloor(vec3 ro, vec3 rd, inout float hitDist, inout vec3 hitColor, inout vec3 hitNormal) {
    vec3 floorColor;
    float t = rayHoneycombFloorIntersection(ro, rd, floorColor);
    if(t < hitDist) {
        hitDist = t;
        hitColor = floorColor;
        hitNormal = vec3(0.0, 1.0, 0.0);
    }
}

// Cylinder functions:
float intersectCylinder(vec3 ro, vec3 rd, vec3 baseCenter, float radius, float height, out int hitType) {
    vec3 oc = ro - baseCenter;
    float tCylinder = INFINITY;
    hitType = 0;
    float a = rd.x * rd.x + rd.z * rd.z;
    float b = 2.0 * (oc.x * rd.x + oc.z * rd.z);
    float c = oc.x * oc.x + oc.z * oc.z - radius * radius;
    if(a > EPSILON) {
        float disc = b * b - 4.0 * a * c;
        if(disc > 0.0) {
            float sqrtDisc = sqrt(disc);
            float t0 = (-b - sqrtDisc) / (2.0 * a);
            float t1 = (-b + sqrtDisc) / (2.0 * a);
            float tCandidate = t0;
            if(tCandidate < EPSILON) tCandidate = t1;
            if(tCandidate > EPSILON) {
                float y = oc.y + tCandidate * rd.y;
                if(y >= 0.0 && y <= height) {
                    tCylinder = tCandidate;
                    hitType = 1;
                }
            }
        }
    }
    if(abs(rd.y) > EPSILON) {
        float tCap = -oc.y / rd.y;
        if(tCap > EPSILON) {
            vec3 hitPos = oc + tCap * rd;
            if(hitPos.x * hitPos.x + hitPos.z * hitPos.z <= radius * radius) {
                if(tCap < tCylinder) {
                    tCylinder = tCap;
                    hitType = 2;
                }
            }
        }
        tCap = (height - oc.y) / rd.y;
        if(tCap > EPSILON) {
            vec3 hitPos = oc + tCap * rd;
            if(hitPos.x * hitPos.x + hitPos.z * hitPos.z <= radius * radius) {
                if(tCap < tCylinder) {
                    tCylinder = tCap;
                    hitType = 3;
                }
            }
        }
    }
    return tCylinder;
}

void addCylinder(vec3 ro, vec3 rd, vec3 baseCenter, float radius, float height, vec3 color,
                 inout float hitDist, inout vec3 hitColor, inout vec3 hitNormal) {
    int hitType;
    float t = intersectCylinder(ro, rd, baseCenter, radius, height, hitType);
    if(t < hitDist) {
        vec3 hitPos = ro + t * rd;
        if(hitType == 1) {
            vec3 n = hitPos - baseCenter;
            n.y = 0.0;
            hitNormal = normalize(n);
        } else if(hitType == 2) {
            hitNormal = vec3(0.0, -1.0, 0.0);
        } else if(hitType == 3) {
            hitNormal = vec3(0.0, 1.0, 0.0);
        }
        hitDist = t;
        hitColor = color;
    }
}

float rayTraceScene(vec3 ro, vec3 rd, out vec3 hitNormal, out vec3 hitColor) {
    float hitDist = INFINITY;
    hitNormal = vec3(0.0);
    addHoneycombFloor(ro, rd, hitDist, hitColor, hitNormal);
    addCylinder(ro, rd, vec3(2.0, 0.0, 2.0), 0.5, 3.0, vec3(1.0, 0.0, 0.0), hitDist, hitColor, hitNormal);
    return hitDist;
}

float calcFresnel(vec3 normal, vec3 inRay) {
    float bias = 0.0;
    float cosTheta = clamp(dot(normal, -inRay), 0.0, 1.0);
    return clamp(bias + pow(1.0 - cosTheta, 2.0), 0.0, 1.0);
}

vec3 calcLighting(vec3 hitPoint, vec3 normal, vec3 inRay, vec3 color) {
    vec3 ambient = vec3(0.1);
    vec3 lightVec = lightPosition - hitPoint;
    vec3 lightDir = normalize(lightVec);
    float lightDist = length(lightVec);
    vec3 shadowNormal, shadowColor;
    float shadowT = rayTraceScene(hitPoint + lightDir * RAY_OFFSET, lightDir, shadowNormal, shadowColor);
    if(shadowT < lightDist) {
        return ambient * color;
    } else {
        float diff = max(dot(normal, lightDir), 0.0);
        vec3 h = normalize(-inRay + lightDir);
        float ndoth = max(dot(normal, h), 0.0);
        float spec = max(pow(ndoth, 50.0), 0.0);
        return min((ambient + vec3(diff)) * color + vec3(spec), vec3(1.0));
    }
}

vec3 shootRayIntoScene(vec3 ro, vec3 rd) {
    vec3 accumColor = vec3(0.0);
    vec3 hitColor;
    vec3 hitNormal;
    float totalWeight = 0.0;
    for(int i = 0; i < maxDepth; i++) {
        float t = rayTraceScene(ro, rd, hitNormal, hitColor);
        if(t >= INFINITY) break;
        vec3 hitPoint = ro + t * rd;
        float fresnel = calcFresnel(hitNormal, rd);
        float weight = (1.0 - fresnel) * (1.0 - totalWeight);
        totalWeight += weight;
        accumColor += calcLighting(hitPoint, hitNormal, rd, hitColor) * weight;
        rd = reflect(rd, hitNormal);
        rd = normalize(rd);
        ro = hitPoint + hitNormal * RAY_OFFSET;
    }
    if(totalWeight > 0.0)
        accumColor /= totalWeight;
    return accumColor;
}

void main() {
    vec3 ro = rayOrigin;
    vec3 rd = normalize(rayDir);
    vec3 accumColor = vec3(0.0);
    float samples = 0.0;
    for(float x = -1.0; x <= 1.0; x += 2.0) {
        for(float y = -1.0; y <= 1.0; y += 2.0) {
            vec3 offset = x * rightOffset + y * upOffset;
            accumColor += shootRayIntoScene(ro + offset, rd);
            samples += 1.0;
        }
    }
    fragColor = vec4(accumColor / samples, 1.0);
}
