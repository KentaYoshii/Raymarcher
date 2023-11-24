#version 330 core
const int MAX_STEPS = 200;
const float SURFACE_DIST = 0.001;
const float SHADOWRAY_OFFSET = 0.007;
const float TEXTURE_EPS = 0.007;
const float PI = 3.14159265;
const int NUM_REFLECTION = 2;

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
    // - to combat against non-rigid body transformation
    float scaleFactor;

    // Material Property
    float shininess;
    float blend;
    vec3 cAmbient;
    vec3 cDiffuse;
    vec3 cSpecular;
    vec3 cReflective;

    // -1 if not used.
    int texLoc;
    float repeatU;
    float repeatV;
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

// =========== Uniforms ============
// Screen/Camera
uniform vec4 eyePosition;
uniform vec2 screenDimensions;
uniform float far;

// Lighting
// - Phong Constants
uniform float ka;
uniform float kd;
uniform float ks;
// - Scene Lights
uniform LightSource lights[10];
uniform int numLights;

// Objects
uniform RayMarchObject objects[50];
uniform int numObjects;

// Textures
uniform sampler2D objTextures[10];

// Options
uniform bool enableGammaCorrection;
uniform bool enableSoftShadow;
uniform bool enableReflection;
// =================================


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

// ================ UV Mapping (non-procedual) ==========
// Cube
vec2 uvMapCube(vec3 p, float repeatU, float repeatV)
{
    float u, v;
    vec3 absP = abs(p);
    float m = max(max(absP.x, absP.y), absP.z);

    // Determine the major axis
    if (m == absP.x)
    {
        u = p.z + 0.5;
        v = p.y + 0.5;
    }
    else if (m == absP.y)
    {
        u = p.x + 0.5;
        v = p.z + 0.5;
    }
    else
    {
        u = p.x + 0.5;
        v = p.y + 0.5;
    }
    return vec2(u * repeatU, v * repeatV);
}

// Cone
vec2 uvMapCone(vec3 p, float repeatU, float repeatV)
{
    // Along the y-axis
    float y = p[1];
    float u, v;
    if (abs(y + 0.5) < TEXTURE_EPS)
    {
        // intersect at flat base
        u = p[0] + 0.5f;
        v = p[2] + 0.5f;
    }
    else
    {
        // intersects at the side
        float theta = atan(p[2], p[0]);
        if (theta < 0)
        {
          u = -theta / (2 * PI);
        }
        else
        {
          u = 1 - (theta / (2 * PI));
        }
        v = y + 0.5f;
    }
    return vec2(u * repeatU, v * repeatV);
}

// Cylinder
vec2 uvMapCylinder(vec3 p, float repeatU, float repeatV)
{
    // Along the y-axis
    float y = p[1];
    float u, v;
    if (abs(y - 0.5) < TEXTURE_EPS)
    {
        u = p[0] + 0.5f;
        v = -p[2] + 0.5f;
    }
    else if (abs(y + 0.5) < TEXTURE_EPS)
    {
        u = p[0] + 0.5f;
        v = p[2] + 0.5f;
    }
    else
    {
       // at the side
       float theta = atan(p[2], p[0]);
       if (theta < 0)
       {
          u = -theta / (2 * PI);
       }
       else
       {
          u = 1 - (theta / (2 * PI));
       }
        v = y + 0.5f;
      }
      return vec2(u * repeatU, v * repeatV);
}

// Sphere
vec2 uvMapSphere(vec3 p, float repeatU, float repeatV)
{
    float u, v;
    // Compute U
    float theta = atan(p[2], p[0]);
    if (theta < 0)
    {
        u = -theta / (2 * PI);
    }
    else
    {
        u = 1 - (theta / (2 * PI));
    }
    // Compute V
    float phi = asin(p[1] / 0.5f);
    v = phi / PI + 0.5f;
    if (v == 0.f || v == 1.f)
    {
       // Poles (singularity)
       u = 0.5f;
    }
    return vec2(u * repeatU, v * repeatV);
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
    vec3 e = vec3(1.0,-1.0, 0)*0.5773*0.0005;
    return normalize(
                e.xyy*sdScene(p + e.xyy).minD +
                e.yyx*sdScene(p + e.yyx).minD +
                e.yxy*sdScene(p + e.yxy).minD +
                e.xxx*sdScene(p + e.xxx).minD
                );
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
    if (abs(closest.minD) < SURFACE_DIST || rayDepth > end) {
        break;
    }
    rayDepth += closest.minD;
  }
  RayMarchRes res;
  if (abs(closest.minD) < SURFACE_DIST) {
      res.intersectObj = closest.minObjIdx;
      // Bruh don't ask me why we need this.
      // Without this, normal calcuation somehow gets messed up
      res.d = rayDepth - 0.001;
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
vec3 getDiffuse(int objId, vec3 p, vec3 n) {
    RayMarchObject obj = objects[objId];
    if (obj.texLoc == -1) {
        return kd * obj.cDiffuse;
    }

    vec2 uv;
    vec3 po = vec3(obj.invModelMatrix * vec4(p, 1.f));
    if (obj.type == CUBE) {
        uv = uvMapCube(po, obj.repeatU, obj.repeatV);
    } else if (obj.type == CONE) {
        uv = uvMapCone(po, obj.repeatU, obj.repeatV);
    } else if (obj.type == CYLINDER) {
        uv = uvMapCylinder(po, obj.repeatU, obj.repeatV);
    } else if (obj.type == SPHERE) {
        uv = uvMapSphere(po, obj.repeatU, obj.repeatV);
    }
    vec2 scaledUV = vec2(obj.scaleFactor * uv[0], obj.scaleFactor * uv[1]);
    vec4 texVal = texture(objTextures[obj.texLoc], uv);
    return (1.f - obj.blend) * kd * obj.cDiffuse + obj.blend * vec3(texVal);
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
vec3 getPhong(vec3 N, int intersectObj, vec3 p, vec3 rd)
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
            L = normalize(-lights[i].lightDir);
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

        // Shadow
        RayMarchRes res = softshadow(p, L, SHADOWRAY_OFFSET, maxT, 8);
        if (res.intersectObj == 1) {
            continue;
        }

        // Diffuse
        float NdotL = dot(N, L);
        if (NdotL < 0) {
            // Pointing away
            continue;
        }
        NdotL = clamp(NdotL, 0.f, 1.f);
        currColor +=  getDiffuse(intersectObj, p, N) * NdotL * lights[i].lightColor;
        // Specular
        vec3 R = reflect(-L, N);
        vec3 dirToCamera = normalize(-rd);
        float RdotV = max(dot(R, dirToCamera), 0.f);
        currColor += getSpecular(RdotV, obj.cSpecular, obj.shininess) * lights[i].lightColor;
        // Add the light source's contribution
        currColor *= fAtt * aFall;
        if (enableSoftShadow) {
            currColor *= res.d;
        }
        total += currColor;
    }

    return total;
}

// =============================================================
vec3 render(inout vec3 ro, inout vec3 rd, inout vec3 ref)
{
    // (note) "inout" allows you to modify ro and rd inside the function

    // Raymarching
    RayMarchRes res = raymarch(ro, rd, far);
    ref = vec3(0);
    if (res.intersectObj == -1) {
        // NO HIT
        return vec3(0.f);
    }
    // HIT
    // - hit point
    vec3 p = ro + rd * res.d;
    // - normalized hit normal
    vec3 pn = getNormal(p);
    // - get color
    vec3 col = getPhong(pn, res.intersectObj, p, rd);
    if (enableGammaCorrection) {
        col = pow(col, vec3(1.0/2.2));
    }

    // Reflection
    // - get reflect dir
    vec3 r = reflect(rd, pn);
    ro = p + pn * SURFACE_DIST * 3.f;
    rd = r;
    // - set material reflectivity
    ref = objects[res.intersectObj].cReflective;
    return col;
}

void main() {
    // Perspective divide and get Ray origin and Ray direction
    vec3 origin = nearClip.xyz / nearClip.w;
    vec3 farC = farClip.xyz / farClip.w;
    vec3 dir = farC - origin;
    dir = normalize(dir);

    // Material reflectiveness
    vec3 ref;
    vec3 fil = vec3(1.);
    // Main render
    vec3 col = render(origin, dir, ref);
    if (enableReflection) {
        // GLSL does not have recursion apparently :(
        // Here is my work around
        // - fil keeps track of the accumulated material reflectivity
        for (int i = 0; i < NUM_REFLECTION; i++) {
            fil *= ref;
            vec3 bounce = ks * fil * render(origin, dir, ref);
            col += bounce;
        }
    }
    fragColor = vec4(col, 1.f);
}
