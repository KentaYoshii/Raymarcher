#version 330 core
// ==== Preprocessor Directives ====
//#define SKY_BACKGROUND
#define DARK_BACKGROUND

// =============== Out =============
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 BrightColor;
// =============== In ==============
in vec4 nearClip;
in vec4 farClip;
in vec2 twoDFragCoord;
// ============ CONST ==============
// - max raymarching steps
const int MAX_STEPS = 256;
const int MAX_STEPS_FRACTALS = 20;
const int FRACTALS_BAILOUT = 2;
// - threshold for intersection
const float SURFACE_DIST = 0.001;
// - small offset for the origin of shadow rays
const float SHADOWRAY_OFFSET = 0.007;
// - small eps for computing uv mapping
const float TEXTURE_EPS = 0.005;
// - area light samples
const int AREA_LIGHT_SAMPLES = 1;
// - PI
const float PI = 3.14159265;
// - reflection depth
const int NUM_REFLECTION = 3;
// - Area Lights
const float LUT_SIZE  = 64.0; // ltc_texture size
const float LUT_SCALE = (LUT_SIZE - 1.0)/LUT_SIZE;
const float LUT_BIAS  = 0.5/LUT_SIZE;
const float ROUGHNESS = 0.5;

// PRIM TYPES
const int CUBE = 0;
const int CONE = 1;
const int CYLINDER = 2;
const int SPHERE = 3;
const int OCTAHEDRON = 4;
const int TORUS = 5;
const int CAPSULE = 6;
const int DEATHSTAR = 7;
const int RECTANGLE = 8;
const int MANDELBROT = 9;
const int MANDELBULB = 10;

// LIGHT TYPES
const int POINT = 0;
const int DIRECTIONAL = 1;
const int SPOT = 2;
const int AREA = 3;

// Bloom
const vec3 BRIGHT_FILTER = vec3(0.2126, 0.7152, 0.0722);
// ============ Structs ============
struct RayMarchObject
{
    // Struct for each shape

    // Type of the shape
    int type;
    // Bring ray to obj space
    mat4 invModelMatrix;
    // Scale
    // - to combat against non-rigid body transformation
    float scaleFactor;

    // Material Property
    float shininess;
    float blend;
    float ior;
    vec3 cAmbient;
    vec3 cDiffuse;
    vec3 cSpecular;
    vec3 cReflective;
    vec3 cTransparent;

    // -1 if not used.
    int texLoc;

    // texture tiling
    float repeatU;
    float repeatV;

    // Area Light
    bool isEmissive;
    vec3 color;
    int lightIdx;
};

struct SceneMin
{
    // Struct to store results of sdScene

    // Object with closest d
    int minObjIdx;
    // Largest step we can make
    float minD;
    // Fractal Trap
    vec4 trap;
};

struct RayMarchRes
{
    // Struct to hold the result for a single Raymarch

    // Id of the intersected object
    int intersectObj;
    // Distance travelled along raydirection
    float d;
    // Fractal Trap
    vec4 trap;
};

struct IntersectionInfo
{
    // Sturct to store intersection information

    // Ray Direction
    vec3 rd;
    // Ray intersection point
    vec3 p;
    // Intersection normal (normalized)
    vec3 n;
    // Intersected object
    int intersectObj;
};

struct LightSource
{
    // Struct for lights in the scene

    // Common
    int type;
    vec3 lightColor;

    vec3 lightDir;
    vec3 lightPos;
    vec3 lightFunc;
    float lightAngle;
    float lightPenumbra;

    // Area Light
    vec3 points[4];
    float intensity;
    bool twoSided;
};

struct RenderInfo
{
    // Struct for holding render results.
    // We need this to differentiate the color values between true hit and environment
    vec3 fragColor;
    bool isEnv;
    bool isAL;
};

// =========== Uniforms ============
// Screen/Camera
uniform vec4 eyePosition;
uniform vec2 screenDimensions;
uniform float far;
uniform bool isTwoD;

// Lighting
// - Phong Constants
uniform float ka;
uniform float kd;
uniform float ks;
uniform float kt;
// - Scene Lights
uniform LightSource lights[10];
uniform int numLights;

// Objects
uniform RayMarchObject objects[30];
uniform int numObjects;

// Textures
uniform sampler2D objTextures[10];
uniform samplerCube skybox;
uniform sampler2D LTC1; // for inverse M
uniform sampler2D LTC2; // GGX norm, fresnel, 0(unused), sphere

// Timer
uniform float iTime;

// Options
uniform bool enableSoftShadow;
uniform bool enableReflection;
uniform bool enableRefraction;
uniform bool enableAmbientOcculusion;
uniform bool enableSkyBox;
uniform float power;
uniform vec2 juliaSeed;

// ================== Utility =======================
// Transforms p along axis by angle
vec3 rotateAxis(vec3 p, vec3 axis, float angle) {
    return mix(dot(axis, p)*axis, p, cos(angle)) + cross(axis,p)*sin(angle);
}

// Applies transformation
vec3 Transform(in vec3 p)
{
//  p = rotateAxis(p, vec3(1, 0, 0), iTime);
  p.xz += sin(iTime);
  return p;
}

// Vector form without project to the plane (dot with the normal)
// Use for proxy sphere clipping
vec3 IntegrateEdgeVec(vec3 v1, vec3 v2)
{
    // Using built-in acos() function will result flaws
    // Using fitting result for calculating acos()
    float x = dot(v1, v2);
    float y = abs(x);

    float a = 0.8543985 + (0.4965155 + 0.0145206*y)*y;
    float b = 3.4175940 + (4.1616724 + y)*y;
    float v = a / b;

    float theta_sintheta = (x > 0.0) ? v : 0.5*inversesqrt(max(1.0 - x*x, 1e-7)) - v;

    return cross(v1, v2)*theta_sintheta;
}

float IntegrateEdge(vec3 v1, vec3 v2)
{
    return IntegrateEdgeVec(v1, v2).z;
}

// P is fragPos in world space (LTC distribution)
vec3 LTC_Evaluate(vec3 N, vec3 V, vec3 P, mat3 Minv, vec3 points[4], bool twoSided)
{
    // construct orthonormal basis around N
    vec3 T1, T2;
    T1 = normalize(V - N * dot(V, N));
    T2 = cross(N, T1);

    // rotate area light in (T1, T2, N) basis
    Minv = Minv * transpose(mat3(T1, T2, N));

    // polygon (allocate 4 vertices for clipping)
    vec3 L[4];
    // transform polygon from LTC back to origin Do (cosine weighted)
    L[0] = Minv * (points[0] - P);
    L[1] = Minv * (points[1] - P);
    L[2] = Minv * (points[2] - P);
    L[3] = Minv * (points[3] - P);

    // use tabulated horizon-clipped sphere
    // check if the shading point is behind the light
    vec3 dir = points[0] - P; // LTC space
    vec3 lightNormal = cross(points[1] - points[0], points[3] - points[0]);
    bool behind = (dot(dir, lightNormal) < 0.0);

    // cos weighted space
    L[0] = normalize(L[0]);
    L[1] = normalize(L[1]);
    L[2] = normalize(L[2]);
    L[3] = normalize(L[3]);

    // integrate
    vec3 vsum = vec3(0.0);
    vsum += IntegrateEdgeVec(L[0], L[1]);
    vsum += IntegrateEdgeVec(L[1], L[2]);
    vsum += IntegrateEdgeVec(L[2], L[3]);
    vsum += IntegrateEdgeVec(L[3], L[0]);

    // form factor of the polygon in direction vsum
    float len = length(vsum);

    float z = vsum.z/len;
    if (behind)
        z = -z;

    vec2 uv = vec2(z*0.5f + 0.5f, len); // range [0, 1]
    uv = uv*LUT_SCALE + LUT_BIAS;

    // Fetch the form factor for horizon clipping
    float scale = texture(LTC2, uv).w;

    float sum = len*scale;
    if (!behind && !twoSided)
        sum = 0.0;

    // Outgoing radiance (solid angle) for the entire polygon
    vec3 Lo_i = vec3(sum, sum, sum);
    return Lo_i;
}
// PBR-maps for roughness (and metallic) are usually stored in non-linear
// color space (sRGB), so we use these functions to convert into linear RGB.
vec3 PowVec3(vec3 v, float p)
{
    return vec3(pow(v.x, p), pow(v.y, p), pow(v.z, p));
}

const float gamma = 2.2;
vec3 ToLinear(vec3 v) { return PowVec3(v, gamma); }
vec3 ToSRGB(vec3 v)   { return PowVec3(v, 1.0/gamma); }

// Simple hash function
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// Function to generate a random 2D vector based on a seed
vec2 random2(vec2 seed) {
    return vec2(hash(seed), hash(seed + vec2(1.0)));
}

vec3 samplePointOnRectangleAreaLight(vec3 lightPos1, vec3 lightPos2, vec3 lightPos3, vec3 lightPos4, vec2 randomUV) {
    // Calculate vectors defining the rectangle
    vec3 side1 = lightPos2 - lightPos1;
    vec3 side2 = lightPos4 - lightPos1;

    // Use barycentric coordinates to interpolate a point on the rectangle
    vec3 randomPoint = lightPos1 + randomUV.x * side1 + randomUV.y * side2;

    return randomPoint;
}

float calculatePower(float iTime, float minPower, float maxPower, float duration)
{
    float t = sin(2.0 * 3.1416 * iTime / duration);
    return mix(minPower, maxPower, 0.5 * (t + 1.0));
}

// ============ Signed Distance Fields ==============
// Define SDF for different shapes here
// - Based on https://iquilezles.org/articles/distfunctions/


// Mandelbrot Set Signed Distance Field
// ref: https://www.shadertoy.com/view/Mss3R8
// @param point in 2D space
float sdMandelBrot(in vec2 p)
{
    float ltime = 0.5-0.5*cos(iTime*0.06);
    float zoom = pow( 0.9, 50.0*ltime );
    vec2  cen = vec2( 0.2655,0.301 ) + zoom*0.8*cos(4.0+2.0*ltime);
    vec2 c = vec2( -0.745, 0.186 ) - 0.045*zoom*(1.0-ltime*0.5);

    float ld2 = 1.0;
    float lz2 = dot(p,p);
    for (int i=0; i < MAX_STEPS; i++) {
        ld2 *= 4.0*lz2;
        p = vec2( p.x*p.x - p.y*p.y, 2.0*p.x*p.y ) + c;
        lz2 = dot(p,p);
        if (lz2 > 200.0) {
            break;
        }
    }
    float d = sqrt(lz2/ld2)*log(lz2);
    return sqrt(clamp((150.0/zoom)*d, 0.0, 1.0));
}

// Mandelbulb Set Signed Distance Field
// Great ref: https://www.youtube.com/watch?v=6IWXkV82oyY&t=1502s
// @param p Point in object space
// @param power (typically 8)
float sdMandelBulb(vec3 pos, out vec4 resColor)
{
    vec3 w = pos;
    float m = dot(w,w);
    vec4 trap = vec4(abs(w),m);
    float dz = 1.0;
    vec3 c = pos;
    // If julia seed is used
    if (length(juliaSeed) != 0) {
        c = vec3(juliaSeed, 0);
    }
    for (int i=0; i < MAX_STEPS_FRACTALS; i++) {
        // derivative
        dz = power * pow(m, (power-1.f)/2.f) * dz + 1.0;
        // z = z^8+c
        float r = length(w);
        float b = power*acos(w.y/r);
        float a = power*atan(w.x, w.z);
        w = c + pow(r,power) *
                vec3(sin(b)*sin(a), cos(b), sin(b)*cos(a));

        trap = min(trap, vec4(abs(w),m));

        m = dot(w,w);
        if( m > FRACTALS_BAILOUT )
            break;
    }
    resColor = vec4(m,trap.yzw);
    // distance estimation (through the Hubbard-Douady potential)
    return 0.25*log(m)*sqrt(m)/dz;
}

// Sphere Signed Distance Field
// @param p Point in object space
// @param r Radius
float sdSphere(vec3 p, float r)
{
  return length(p)-r;
}

// Box Signed Distance Field
// @param p Point in object space
// @param b half-length dimensions of the box (x,y,z)
float sdBox(vec3 p, vec3 b)
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

// Cone Signed Distance Field
// @param p Point in object space
// @param r Radius of the base
// @param h Half height of the cone
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

// Cylinder Signed Distance Field
// @param p Point in object space
// @param h Half height of the cone
// @param r Radius of the base
float sdCylinder(vec3 p, float h, float r)
{
  vec2 d = abs(vec2(length(p.xz),p.y)) - vec2(r,h);
  return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}

// Octahedron Signed Distance Field
// @param p Point in object space
// @param s radius s
float sdOctahedron(vec3 p, float s)
{
    p = abs(p);
    float m = p.x + p.y + p.z - s;
    vec3 r = 3.0*p - m;
    vec3 q;
    if( r.x < 0.0 ) q = p.xyz;
    else if( r.y < 0.0 ) q = p.yzx;
    else if( r.z < 0.0 ) q = p.zxy;
    else return m*0.57735027;
    float k = clamp(0.5*(q.z-q.y+s),0.0,s);
    return length(vec3(q.x,q.y-s+k,q.z-k));
}

// Torus Signed Distance Field
// @param p Point in object space
// @param t play around but 1:4 ratio works well
float sdTorus(vec3 p, vec2 t)
{
    vec2 q = vec2(length(p.xz)-t.x,p.y);
    return length(q)-t.y;
}

// Capsule Signed Distance Field
// @param p Point in object space
// @param h half height
// @param r corenr radius of capsule
float sdCapsule(vec3 p, float h, float r)
{
  p.y -= clamp(p.y, 0.0, h);
  return length(p) - r;
}

// Deathstar Signed Distance Field
// @param p2 Point in object space
// @param ra,rb,d play around
float sdDeathStar( in vec3 p2, in float ra, float rb, in float d )
{
    vec2 p = vec2( p2.x, length(p2.yz) );

    float a = (ra*ra - rb*rb + d*d)/(2.0*d);
    float b = sqrt(max(ra*ra-a*a,0.0));
    if( p.x*b-p.y*a > d*max(b-p.y,0.0) )
    {
        return length(p-vec2(a,b));
    }
    else
    {
        return max( (length(p          )-ra),
                   -(length(p-vec2(d,0))-rb));
    }
}

// Given a point in object space and type of the SDF
// Invoke the appropriate SDF function and return the distance
// @param p Point in object space
// @param type Type of the object
float sdMatch(vec3 p, int type, int id, out vec4 trapCol)
{
    if (type == CUBE) {
        return sdBox(p, vec3(0.5));
    } else if (type == CONE) {
        return sdCone(p, 0.5, 0.5);
    } else if (type == CYLINDER) {
        return sdCylinder(p, 0.5, 0.5);
    } else if (type == SPHERE) {
        return sdSphere(p, 0.5);
    } else if (type == OCTAHEDRON) {
        return sdOctahedron(p, 0.5);
    } else if (type == TORUS) {
        return sdTorus(p, vec2(0.5, 0.5/4));
    } else if (type == CAPSULE) {
        return sdCapsule(p, 0.5, 0.1);
    } else if (type == DEATHSTAR) {
        return sdDeathStar(p, 0.5, 0.35, 0.5);
    } else if (type == RECTANGLE) {
        // in 2d
        return sdBox(p, vec3(0.5, 0.5, 0));
    } else if (type == MANDELBROT) {
        return sdMandelBrot(vec2(p));
    } else if (type == MANDELBULB) {
        // For animating Mandelbulb
        // float power = calculatePower(iTime, 2.f, 20.f, 20.f);
        // vec3 c = vec3(-0.5 * cos(iTime), 0.4 * sin(iTime), 0.4 * cos(iTime));
        return sdMandelBulb(p, trapCol);
    }
}

// ================ UV Mapping (non-procedual) ==========
// uv mapping for simple primitives

// uv Mapping for Cube
vec2 uvMapCube(vec3 p, float repeatU, float repeatV)
{
    float u, v;
    vec3 absP = abs(p);
    float m = max(max(absP.x, absP.y), absP.z);

    // Determine the major axis
    if (m == absP.x) {
        if (p.x < 0) {
            u = p.z + 0.5;
            v = p.y + 0.5;
        } else {
            u = -p.z + 0.5f;
            v = p.y + 0.5f;
        }
    } else if (m == absP.y) {
        if (p.y < 0) {
            u = p.x + 0.5;
            v = p.z + 0.5;
        } else {
            u = p.x + 0.5f;
            v = -p.z + 0.5f;
        }
    } else {
        if (p.z < 0) {
            u = -p.x + 0.5f;
            v = p.y + 0.5f;
        } else {
            u = p.x + 0.5f;
            v = p.y + 0.5f;
        }
    }
    return vec2(u * repeatU, v * repeatV);
}

// uvMapping for Cone
vec2 uvMapCone(vec3 p, float repeatU, float repeatV)
{
    // Along the y-axis
    float y = p[1];
    float u, v;
    if (abs(y + 0.5) < TEXTURE_EPS) {
        // intersect at flat base
        u = p[0] + 0.5f;
        v = p[2] + 0.5f;
    } else {
        // intersects at the side
        float theta = atan(p[2], p[0]);
        if (theta < 0) {
          u = -theta / (2 * PI);
        } else {
          u = 1 - (theta / (2 * PI));
        }
        v = y + 0.5f;
    }
    return vec2(u * repeatU, v * repeatV);
}

// uvMapping for Cylinder
vec2 uvMapCylinder(vec3 p, float repeatU, float repeatV)
{
    // Along the y-axis
    float y = p[1];
    float u, v;
    if (abs(y - 0.5) < TEXTURE_EPS) {
        u = p[0] + 0.5f;
        v = -p[2] + 0.5f;
    } else if (abs(y + 0.5) < TEXTURE_EPS) {
        u = p[0] + 0.5f;
        v = p[2] + 0.5f;
    } else {
       // at the side
       float theta = atan(p[2], p[0]);
       if (theta < 0) {
          u = -theta / (2 * PI);
       } else {
          u = 1 - (theta / (2 * PI));
       }
        v = y + 0.5f;
      }
      return vec2(u * repeatU, v * repeatV);
}

// uvMapping for Sphere
vec2 uvMapSphere(vec3 p, float repeatU, float repeatV)
{
    float u, v;
    // Compute U
    float theta = atan(p[2], p[0]);
    if (theta < 0) {
        u = -theta / (2 * PI);
    } else {
        u = 1 - (theta / (2 * PI));
    }
    // Compute V
    float phi = asin(p[1] / 0.5f);
    v = phi / PI + 0.5f;
    if (v == 0.f || v == 1.f) {
       // Poles (singularity)
       u = 0.5f;
    }
    return vec2(u * repeatU, v * repeatV);
}

// ================ Raymarch Algorithm ==================
// Union of all the SDFs in the scene
// @param p Current raymarching point for which we wish to
// find the distance
// @returns SceneMin struct with closest distance and closest
// object
SceneMin sdScene(vec3 p){
    float minD = 1000000.f;
    int minObj = -1;
    float currD;
    vec3 po;
    vec4 trapCol;
    for (int i = 0; i < numObjects; i++) {
        // Get current obj
        RayMarchObject obj = objects[i];
        // Conv to Object space
        // - (Note) for global transformation
        // po = vec3(obj.invModelMatrix * vec4(Transform(p), 1.f));
        po = vec3(obj.invModelMatrix * vec4(p, 1.f));

        // Get the distance to the object
        currD = sdMatch(po, obj.type, i, trapCol) * obj.scaleFactor;
        if (currD < minD) {
            // Update if we found a closer object
            minD = currD;
            minObj = i;
        }
    }
    // Populate the struct
    SceneMin res;
    res.minD = minD;
    res.minObjIdx = minObj;
    res.trap = trapCol;
    return res;
}

// Given intersection point, get the normal
// - https://iquilezles.org/articles/normalsSDF
// @param p Intersection point
// @returns normalized intersection point normal
vec3 getNormal(in vec3 p) {
    vec3 e = vec3(1.0,-1.0, 0)*0.5773*0.0005;
    return normalize(
                e.xyy*sdScene(p + e.xyy).minD +
                e.yyx*sdScene(p + e.yyx).minD +
                e.yxy*sdScene(p + e.yxy).minD +
                e.xxx*sdScene(p + e.xxx).minD
                );
}

// Performs raymarching
// @param ro Ray origin
// @param rd Ray direction
// @param end Far plane
// @param side Determines if we are inside or outside the object
// - used in refraction
// @returns structs that contains the result of raymarching
RayMarchRes raymarch(vec3 ro, vec3 rd, float end, float side) {
  // Start from eye pos
  float rayDepth = 0.0;
  SceneMin closest;
  closest.minD = 100;
  // Start the march
  for(int i = 0; i < MAX_STEPS; i++) {
    // Get the point
    vec3 p = ro + rd * rayDepth;
    // Find the closest object in the scene
    closest = sdScene(p);
    if (abs(closest.minD) < SURFACE_DIST || rayDepth > end) {
        // If hit or exceed the far plane, break
        break;
    }
    // March the ray
    rayDepth += closest.minD * side;
  }
  RayMarchRes res;
  if (abs(closest.minD) < SURFACE_DIST) {
      // HIT
      res.intersectObj = closest.minObjIdx;
      // Bruh don't ask me why we need this.
      // Without this, normal calcuation somehow gets messed up
      res.d = rayDepth - 0.001;
      res.trap = closest.trap;
  } else {
      // NO HIT
      res.intersectObj = -1;
  }
  return res;
}

// ================== Phong Total Illumination =================
// Area Light Stuff
// - mostly black magic


// Computes the shadow scale for soft shadow
// - https://iquilezles.org/articles/rmshadows/
// @param ro Ray origin
// @param rd Ray direction
// @param mint Starting t
// @param maxt End t
// @param k How "hard" we want the shadow to be
// @retunrs Result of raymarching
RayMarchRes softshadow(vec3 ro, vec3 rd, float mint, float maxt, float k )
{
    float res = 1.0;
    float rayDepth = mint;
    RayMarchRes r;
    SceneMin closest;
    for(int i=0; i < MAX_STEPS; i++)
    {
        closest = sdScene(ro + rd*rayDepth);
        if(abs(closest.minD) < SURFACE_DIST || rayDepth > maxt) {
           break;
        }
        res = min(res, k * closest.minD/(rayDepth));
        // March the ray
        rayDepth += abs(closest.minD);
    }
    if (abs(closest.minD) < SURFACE_DIST) {
        // HIT
        r.intersectObj = closest.minObjIdx;
        r.d = res;
    } else {
        // NO HIT
        r.intersectObj = -1;
    }
    return r;
}

// Calculate the ambient occlusion
// https://iquilezles.org/articles/nvscene2008/rwwtt.pdf
float calcAO(in vec3 pos, in vec3 nor)
{
    float occ = 0.0;
    float sca = 1.0;
    for (int i=0; i<5; i++) {
        float h = 0.01 + 0.12*float(i)/4.0;
        float d = sdScene(pos + h*nor).minD;
        occ += (h-d)*sca;
        sca *= 0.95;
        if( occ>0.35 ) break;
    }
    return clamp( 1.0 - 3.0*occ, 0.0, 1.0 ) * (0.5+0.5*nor.y);
}

// Gets the diffuse term
// @param objId Id of the intersected object
// @param p Intersection Point in world space
// @param n Normal
vec3 getDiffuse(int objId, vec3 p, vec3 n) {
    RayMarchObject obj = objects[objId];
    if (obj.texLoc == -1) {
        // No texture used
        return kd * obj.cDiffuse;
    }
    // Texture used -> find uv
    vec2 uv;
    // (Note) For global transformation
    // vec3 po = vec3(obj.invModelMatrix * vec4(Transform(p), 1.f));
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
    // Sample
    vec4 texVal = texture(objTextures[obj.texLoc], uv);
    // Linear interpolate
    return (1.f - obj.blend) * kd * obj.cDiffuse + obj.blend * vec3(texVal);
}

// Gets the specular term (special case when shininess = 0)
// @param RdotV R dot V
// @param cspec Specular component of the object
// @param shiniess
vec3 getSpecular(float RdotV, vec3 cspec, float shi) {
    if (shi == 0) {
        return ks * RdotV * cspec;
    }
    return ks * pow(RdotV, shi) * cspec;
}

// Gets the angular fall off term
float angularFalloffFactor(float angle, float innerA, float outerA) {
    float t = (angle - innerA) / (outerA - innerA);
    return -2 * pow(t, 3) + 3 * pow(t, 2);
}

// Gets the attenuation factor
float attenuationFactor(float d, vec3 func) {
    return min(1.f / (func[0] + (d * func[1]) + (d * d * func[2])), 1.f);
}

// Get Area Light
// @param N Normal
// @param V View vector
// @param P Intersection point
// @param lightIdx Light index of the area light
// @param objIdx Intersected object's index
// @returns vec3 of area light's contribution
vec3 getAreaLight(vec3 N, vec3 V, vec3 P, int lightIdx, int objIdx)
{
    float dotNV = clamp(dot(N, V), 0.0f, 1.0f);
    // use roughness and sqrt(1-cos_theta) to sample M_texture
    vec2 uv = vec2(0, sqrt(1.0f - dotNV));
    uv = uv*LUT_SCALE + LUT_BIAS;
    // get 4 parameters for inverse_M
    vec4 t1 = texture(LTC1, uv);
    // Get 2 parameters for Fresnel calculation
    vec4 t2 = texture(LTC2, uv);
    mat3 Minv = mat3(
        vec3(t1.x, 0, t1.y),
        vec3(  0,  1,    0),
        vec3(t1.z, 0, t1.w)
    );

    LightSource areaLight = lights[lightIdx];
    RayMarchObject obj = objects[objIdx];
    vec3 mDiffuse = obj.cDiffuse;
    vec3 mSpecular = obj.cSpecular;
    // Evaluate LTC shading
    vec3 diffuse = LTC_Evaluate(N, V, P, mat3(1), areaLight.points, areaLight.twoSided);
    vec3 specular = LTC_Evaluate(N, V, P, Minv, areaLight.points, areaLight.twoSided);
    // GGX BRDF shadowing and Fresnel
    specular *= mSpecular*t2.x + (areaLight.intensity - mSpecular) * t2.y;
    vec3 col = areaLight.lightColor * 1.0 * (specular + mDiffuse * diffuse);
    return col;
}

// Gets Phong Light
// @param N normal
// @param intersectObj Id of the intersected object
// @param p Intersection point
// @param rd Ray direction
// @returns phong color for that fragment
vec3 getPhong(vec3 N, int intersectObj, vec3 p, vec3 rd)
{
    vec3 total = vec3(0.f);
    RayMarchObject obj = objects[intersectObj];

    // Ambience
    float ao = 1.f;
    if (enableAmbientOcculusion) {
        ao = calcAO(p, N);
    }
    total += obj.cAmbient * ka * ao;

    // Loop Lights
    for (int i = 0; i < numLights; i++) {
        float fAtt = 1.f;
        float aFall = 1.f;
        float d = length(p - lights[i].lightPos);
        vec3 currColor = vec3(0.f);
        vec3 L;
        float maxT;
        vec3 center;
        if (lights[i].type == POINT) {
            L = normalize(lights[i].lightPos - p);
            fAtt = attenuationFactor(d, lights[i].lightFunc);
            maxT = length(lights[i].lightPos - p);
        } else if (lights[i].type == DIRECTIONAL) {
            L = normalize(-lights[i].lightDir);
            maxT = far;
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

        vec3 V = normalize(-rd);
        // Area Light Calculation
        if (lights[i].type == AREA) {
            vec3 areaColor = vec3(0.f);
            vec3 p1 = lights[i].points[0],
                 p2 = lights[i].points[1],
                 p3 = lights[i].points[2],
                 p4 = lights[i].points[3];
            for (int idx = 0; idx < AREA_LIGHT_SAMPLES; idx++) {
                // Sample a point and cast a shadow ray towards it
                vec3 randomP = samplePointOnRectangleAreaLight(p1,p2,p3,p4,vec2(rd + idx));
                L = normalize(randomP - p);
                float NdotL = dot(N, L);
                if (NdotL <= 0.005f) {
                    // Pointing away
                    continue;
                }
                maxT = length(randomP - p);
                // Check for shadow
                RayMarchRes res = softshadow(p + N * SURFACE_DIST, L, 0, maxT, 8);
                if (res.intersectObj != -1) {
                    // Shadow Ray intersected an object
                    // We need to check if the intersected object
                    // is indeed area light or not
                    if (objects[res.intersectObj].lightIdx != i) {
                        // Intersected others
                        continue;
                    }
                }
                // calculate light contribution
                areaColor += getAreaLight(N, V, p, i, intersectObj);
            }
            currColor += areaColor / AREA_LIGHT_SAMPLES;
        } else {
            // Shadow
            RayMarchRes res = softshadow(p + N * SURFACE_DIST, L, 0, maxT, 8);
            if (res.intersectObj != -1) {
                // Shadow ray intersect
                continue;
            }
            // Diffuse
            float NdotL = dot(N, L);
            if (NdotL <= 0.005f) {
                // Pointing away
                continue;
            }
            NdotL = clamp(NdotL, 0.f, 1.f);
            currColor +=  getDiffuse(intersectObj, p, N) * NdotL * lights[i].lightColor;

            // Specular
            vec3 R = reflect(-L, N);
            float RdotV = clamp(dot(R, V), 0.f, 1.f);
            currColor += getSpecular(RdotV, obj.cSpecular, obj.shininess) * lights[i].lightColor;
            // Add the light source's contribution
            currColor *= fAtt * aFall;
            if (enableSoftShadow) {
                currColor *= res.d;
            }
        }

        total += currColor;
    }
    return total;
}

// Extract very bright fragments to feed into the color attachment 1
// which will later be used by Bloom
// @param colro fragment color in question
void setBrightness(vec3 color) {
    float brightness = dot(color.rgb, BRIGHT_FILTER);
    if (brightness > 1.0) {
        BrightColor = vec4(color.rgb, 1.0);
    }
    else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}

vec3 getSky(vec3 rd) {
    vec3 col  = vec3(0.8,.9,1.1)*(0.6+0.4*rd.y);
    col += 5.0*vec3(0.8,0.7,0.5)*
            pow(clamp(
                    dot(rd,vec3(0.577, 0.577, -0.577)),
                    0.0,1.0),
                32.0 );
    return col;
}

// =============================================================
// Given ray origin and ray direction, performs a raymarching
// @param ro Ray origin
// @param rd Ray direction
// @param i IntersectionInfo we are populating
// @param side Determines if we are inside or outside of an object (for refraction)
RenderInfo render(in vec3 ro, in vec3 rd, out IntersectionInfo i, in float side)
{
    RenderInfo ri;
    // Raymarching
    RayMarchRes res = raymarch(ro, rd, far, side);
    if (res.intersectObj == -1) {
        // If no hit but sky box is used, sample
        if (enableSkyBox) {
            ri.fragColor = vec3(texture(skybox, rd).rgb);
        } else {
            // Simple black screen
#ifdef SKY_BACKGROUND
            ri.fragColor = getSky(rd);

#else
            ri.fragColor = vec3(0.f);
#endif
        }
        ri.isAL = false;
        ri.isEnv = true;
        return ri;
    }

    // HIT
    // - hit point
    vec3 p = ro + rd * res.d;
    // - normalized hit normal
    vec3 pn = getNormal(p);
    // - get color
    vec3 col;
    if (objects[res.intersectObj].isEmissive) {
        // Area Light
        col = objects[res.intersectObj].color;
        ri.isAL = true;
    } else {
        if (objects[res.intersectObj].type == MANDELBULB) {
            // Orbit Trap to color
            col = vec3(0.2);
            col = mix( col, vec3(0.10,0.20,0.30), clamp(res.trap.y,0.0,1.0) );
            col = mix( col, vec3(0.02,0.10,0.30), clamp(res.trap.z*res.trap.z,0.0,1.0) );
            col = mix( col, vec3(0.30,0.10,0.02), clamp(pow(res.trap.w,6.0),0.0,1.0) );
            col *= 0.5;
            col *= getPhong(pn, res.intersectObj, p, rd) * 8;
        } else {
            col = getPhong(pn, res.intersectObj, p, rd);
        }
        ri.isAL = false;
    }

    // set output variable
    i.p = ro + rd * res.d;
    i.n = pn;
    i.rd = rd;
    i.intersectObj = res.intersectObj;

    // set return
    ri.fragColor = col;
    ri.isEnv = false;

    return ri;
}

void main() {

    if (isTwoD) {
        vec2 coord = twoDFragCoord.xy;
        float scol = sdMandelBrot(coord);
        vec3 vcol = pow( vec3(scol), vec3(0.9,1.1,1.4) );
        fragColor = vec4(vec3(vcol), 1.f);
        return;
    }

    // Perspective divide and get Ray origin and Ray direction
    // - why we need this? refer to the ref in vertex shader
    vec3 origin = nearClip.xyz / nearClip.w;
    vec3 farC = farClip.xyz / farClip.w;
    vec3 dir = farC - origin;
    dir = normalize(dir);

    vec3 phong = vec3(0.),
         refl = vec3(0.),
         refr = vec3(0.);

    IntersectionInfo info,
            originalInfo;

    // Main render
    RenderInfo ri = render(origin, dir, info, 1.f);
    if (ri.isEnv) {
       BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
       fragColor = vec4(ri.fragColor, 1.f);
       return;
    } else if (ri.isAL) {
       setBrightness(ri.fragColor);
       fragColor = vec4(ri.fragColor, 1.f);
       return;
    }

    phong = ri.fragColor;

    // Copy
    originalInfo.intersectObj = info.intersectObj;
    originalInfo.n = info.n;
    originalInfo.p = info.p;
    originalInfo.rd = info.rd;

    RayMarchObject obj = objects[info.intersectObj];
    if (enableReflection && length(obj.cReflective) != 0) {
        vec3 fil = vec3(1.f);
        // GLSL does not have recursion apparently :(
        // Here is my work around
        // - fil keeps track of the accumulated material reflectivity
        for (int i = 0; i < NUM_REFLECTION; i++) {
            // Reflect ray
            vec3 r = reflect(info.rd, info.n);
            vec3 shiftedRO = info.p + r * SURFACE_DIST * 3.f;
            fil *= objects[info.intersectObj].cReflective;
            // Render the reflected ray
            RenderInfo res = render(shiftedRO, r, info, 1.f);
            // Always want to incorporate the first reflected ray's color val
            vec3 bounce = ks * fil * res.fragColor;
            refl += bounce;
            if (res.isEnv) {
                // If reflected ray did not hit anything, we break
                // since we cannot reflect off a skybox/void
                break;
            }
        }
    }

    if (enableRefraction && length(obj.cTransparent) != 0) {
        // No recursion so hardcoded 2 refractions :(
        // also it does not account for the reflected light contributions
        // of the refracted rays (again, no recursion) so this is really wrong.
        // I was able to find some article about backward raytracing that cleverly
        // gets around this but, like many other things, for the time being this is
        // good enough.

        float ior = objects[originalInfo.intersectObj].ior;
        vec3 ct = objects[originalInfo.intersectObj].cTransparent;

        // Air -> Medium
        // - air ior is 1.
        vec3 rdIn = refract(originalInfo.rd, originalInfo.n, 1./ior);
        vec3 pEnter = originalInfo.p - originalInfo.n * SURFACE_DIST * 3.f;
        float dIn = raymarch(pEnter, rdIn, far, -1.).d;

        vec3 pExit = pEnter + rdIn * dIn;
        vec3 nExit = -getNormal(pExit);

        vec3 rdOut = refract(rdIn, nExit, ior);
        if (length(rdOut) == 0) {
            // Total Internal Reflection
            refr = vec3(0.);
        } else {
            refr += kt * ct * render(pExit - nExit * SURFACE_DIST*3.f, rdOut, info, 1.).fragColor;
        }
    }

    vec3 col = phong + refl + refr;
    setBrightness(col);
    fragColor = vec4(col, 1.f);
}
