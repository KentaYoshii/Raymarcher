#version 330 core
const int MAX_STEPS = 150;
const float SURFACE_DIST = 0.001;
const float eps = 0.01;

// PRIM TYPES
const int CUBE = 0;
const int CONE = 1;
const int CYLINDER = 2;
const int SPHERE = 3;

// LIGHT TYPES
const int POINT = 0;
const int DIRECTIONAL = 1;
const int SPOT = 2;

// ============ Structs ============
// Struct for each shape
struct RayMarchObject
{
    // Type of the concrete shape
    int type;
    // Bring point to obj space
    mat4 invModelMatrix;
    // Scale
    float scaleFactor;

    // Material Property
    float shininess;
    vec3 cAmbient;
    vec3 cDiffuse;
    vec3 cSpecular;
};

// Struct to store results of sdScene
struct SceneMin
{
    int minObjIdx;
    float minD;
};

// Struct to hold the result for RayMarch
struct RayMarchRes
{
    int intersectObj;
    float d;
};

// Struct for lights in the scene
struct LightSource
{
    int type;
    vec3 lightPos;
    vec3 lightDir;
    vec3 lightColor;
    vec3 lightFunc;
    float lightAngle;
    float lightPenumbra;
};
// =================================


// =============== In ==============
in vec4 nearClip;
in vec4 farClip;
// =================================



// =============== Out =============
out vec4 fragColor;
// =================================

// Uniforms
uniform vec4 eyePosition;
uniform vec2 screenDimensions;
uniform RayMarchObject objects[50];
uniform LightSource lights[10];
uniform int numObjects;
uniform int numLights;
uniform float far;

// Constants
uniform float ka;
uniform float kd;
uniform float ks;

// ============ Signed Distance Fields ==============
// Define SDF for different shapes here
// - Based on https://iquilezles.org/articles/distfunctions/

// Sphere
float sdSphere(vec3 p, float r)
{
  return length(p)-r;
}

// Box
float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

// Cone
float sdCone(vec3 p, float r, float h)
{
    vec2 po = vec2(length(p.xz) - r, p.y + h);
    vec2 e = vec2(-r, 2.0 * h);
    vec2 q = po - e * clamp(dot(po, e) / dot(e, e), 0.0, 1.0);
    float d = length(q);
    if (max(q.x, q.y) > 0.0) {
        return d;
    }
    return -min(d, po.y);
}

// Cylinder
float sdCylinder( vec3 p, float h, float r )
{
  vec2 d = abs(vec2(length(p.xz),p.y)) - vec2(r,h);
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}


float sdMatch(vec3 p, int type)
{
    float d = 0.f;
    if (type == CUBE) {
        d = sdBox(p, vec3(0.5));
    } else if (type == CONE) {
        d = sdCone(p, 0.5, 0.5);
    } else if (type == CYLINDER) {
        d = sdCylinder(p, 0.5, 0.5);
    } else if (type == SPHERE) {
        d = sdSphere(p, 0.5);
    }
    return d;
}

// ================ Raymarch Algorithm ==================

// Union of all SDFs
SceneMin sdScene(vec3 p){
    float minD = 1000000.f;
    int minObj = -1;
    float currD;
    vec3 po;
    for (int i = 0; i < numObjects; i++) {
        // Get current obj
        RayMarchObject obj = objects[i];
        // Conv to Object space
        po = vec3(obj.invModelMatrix * vec4(p, 1.f));
        currD = sdMatch(po, obj.type) * obj.scaleFactor;
        if (currD < minD) {
            minD = currD;
            minObj = i;
        }
    }
    SceneMin res;
    res.minD = minD;
    res.minObjIdx = minObj;
    return res;
}

// Given intersection point, get the normal
vec3 getNormal(in vec3 p) {
    const vec3 small_step = vec3(0.001, 0.0, 0.0);

    float gradient_x = sdScene(p + small_step.xyy).minD - sdScene(p - small_step.xyy).minD;
    float gradient_y = sdScene(p + small_step.yxy).minD - sdScene(p - small_step.yxy).minD;
    float gradient_z = sdScene(p + small_step.yyx).minD - sdScene(p - small_step.yyx).minD;

    vec3 normal = vec3(gradient_x, gradient_y, gradient_z);

    return normalize(normal);
}

RayMarchRes raymarch(vec3 ro, vec3 rd, float end) {
  // Start from eye pos
  float rayDepth = 0.0;
  SceneMin closest;
  closest.minD = 100;
  // Start the march
  for(int i = 0; i < MAX_STEPS; i++) {
    vec3 p = ro + rd * rayDepth;
    closest = sdScene(p);
    rayDepth += closest.minD;
    if (closest.minD < SURFACE_DIST || rayDepth >= end) {
        break;
    }
  }
  RayMarchRes res;
  if (closest.minD < SURFACE_DIST) {
      res.intersectObj = closest.minObjIdx;
      res.d = rayDepth;
  } else {
      res.intersectObj = -1;
  }
  return res;
}

// =============================================================

// ================== Phong Total Illumination =================
RayMarchRes softshadow(vec3 ro, vec3 rd, float mint, float maxt, float k )
{
    float res = 1.0;
    float rayDepth = mint;
    RayMarchRes r;
    for( int i=0; i<MAX_STEPS && rayDepth<maxt; i++ )
    {
        float h = sdScene(ro + rd*rayDepth).minD;
        if( h<SURFACE_DIST ) {
            r.d = 0.0;
            r.intersectObj = 1;
            return r;
        }
        res = min( res, k * h/(rayDepth));
        rayDepth += h;
    }
    r.intersectObj = -1;
    r.d = res;
    return r;
}

// Get the diffuse term
vec3 getDiffuse(float NdotL, vec3 cdiff) {
    return kd * NdotL * cdiff;
}

// Get the specular term (special case when shininess = 0)
vec3 getSpecular(float RdotV, vec3 cspec, float shi) {
    if (shi == 0) {
        return ks * RdotV * cspec;
    }
    return ks * pow(RdotV, shi) * cspec;
}

// Get the angular fall off term
float angularFalloffFactor(float angle, float innerA, float outerA) {
    float t = (angle - innerA) / (outerA - innerA);
    return -2 * pow(t, 3) + 3 * pow(t, 2);
}

// Get the attenuation factor
float attenuationFactor(float d, vec3 func) {
    return min(1.f / (func[0] + (d * func[1]) + (d * d * func[2])), 1.f);
}

// Get Phong Light
vec3 getPhong(vec3 N, int intersectObj, vec3 p)
{
    vec3 total = vec3(0.f);
    RayMarchObject obj = objects[intersectObj];

    // Ambience
    total += obj.cAmbient * ka;

    // Loop Lights
    for (int i = 0; i < numLights; i++) {
        float fAtt = 1.f;
        float aFall = 1.f;
        float d = length(p - lights[i].lightPos);
        vec3 currColor = vec3(0.f);
        vec3 L;
        float maxT;
        if (lights[i].type == POINT) {
            L = normalize(lights[i].lightPos - p);
            fAtt = attenuationFactor(d, lights[i].lightFunc);
            maxT = length(lights[i].lightPos - p);
        } else if (lights[i].type == DIRECTIONAL) {
            L = -normalize(lights[i].lightDir);
            maxT = 100.f;
        } else if (lights[i].type == SPOT) {
            L = normalize(lights[i].lightPos - p);
            fAtt = attenuationFactor(d, lights[i].lightFunc);
            maxT = length(lights[i].lightPos - p);
            // Angular Falloff
            float cosalpha = dot(-normalize(lights[i].lightDir), L);
            float inner = lights[i].lightAngle - lights[i].lightPenumbra;
            if (cosalpha <= cos(lights[i].lightAngle)){
                aFall = 0.f;
            } else if (cosalpha > cos(inner)) {
                aFall = 1.f;
            } else {
                aFall =  1.f -
                        angularFalloffFactor(acos(cosalpha), inner, lights[i].lightAngle);
             }
        }

        // Soft Shadow
        RayMarchRes res = softshadow(p, L, eps, maxT, 64);
        if (res.intersectObj == 1) {
            continue;
        }

        // Diffuse
        float NdotL = dot(N, L);
        if (NdotL < 0.f) {
            continue;
        }
        NdotL = clamp(dot(N, L), 0.f, 1.f);
        currColor +=  getDiffuse(NdotL, obj.cDiffuse);
        // Specular
        vec3 R = reflect(-L, N);
        vec3 dirToCamera = normalize(vec3(eyePosition) - p);
        float RdotV = clamp(dot(R, dirToCamera), 0.f, 1.f);
        currColor += getSpecular(RdotV, obj.cSpecular, obj.shininess);
        // Add the light source's contribution
        currColor *= fAtt * lights[i].lightColor * aFall;
        total += currColor * res.d;
    }

    return total;
}

// =============================================================

void main() {
    // Perspective divide
    vec3 origin = nearClip.xyz / nearClip.w;
    vec3 farC = farClip.xyz / farClip.w;
    vec3 dir = farC - origin;
    dir = normalize(dir);

    // Raymarching
    RayMarchRes res = raymarch(origin, dir, far);
    if (res.intersectObj != -1) {
        // HIT
        vec3 sp = origin + dir * res.d;
        vec3 sn = getNormal(sp);
        fragColor = vec4(getPhong(sn, res.intersectObj, sp), 1.f);
    } else {
        // NO HIT
        fragColor = vec4(vec3(0.f), 1.f);
    }
}
