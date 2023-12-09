#version 330 core
// ==== Preprocessor Directives ====
#define SKY_BACKGROUND
// #define DARK_BACKGROUND
// #define WHITE_BACKGROUND
// #define CLOUD
// #define TERRAIN
#define SEA

// =============== Out =============
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 BrightColor;
// =============== In ==============
in vec4 nearClip;
in vec4 farClip;
in vec2 twoDFragCoord;
// ============ CONST ==============
const float INSIDE = -1.f;
const float OUTSIDE = 1.f;
// - max raymarching steps
const int MAX_STEPS = 256;
const int MAX_STEPS_FRACTALS = 20;
const int FRACTALS_BAILOUT = 2;
// - threshold for intersection
const float SURFACE_DIST = 0.001;
const float PLANCK = 0.01;
// - small offset for the origin of shadow rays
const float SHADOWRAY_OFFSET = 0.007;
// - small eps for computing uv mapping
const float TEXTURE_EPS = 0.005;
// - area light samples
const int AREA_LIGHT_SAMPLES = 1;
// - PI
const float PI = 3.14159265;
const float TAU = 6.28318;
const vec3 ANGLE = vec3(0);
// - reflection depth
const int NUM_REFLECTION = 1;
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
const int MENGERSPONGE = 11;
const int SIERPINSKI = 12;

// CUSTOM SCENE
const int CUSTOM = 13;

// LIGHT TYPES
const int POINT = 0;
const int DIRECTIONAL = 1;
const int SPOT = 2;
const int AREA = 3;

// Bloom
const vec3 BRIGHT_FILTER = vec3(0.2126, 0.7152, 0.0722);

// TERRAIN
const float TERRAIN_HIGH = 700.f;


// CLOUD
const float CLOUD_STEP_SIZE = 0.3f;
const float ABSORPTION_COEFFICIENT = 0.5;
const vec3 CLOUD_DIFFUSE = vec3(0.8f);
const vec3 CLOUD_AMBIENT = vec3(0.03, 0.018, 0.018);
const float CLOUD_LOW = 600.f;
const float CLOUD_MID = 900.f;
const float CLOUD_HIGH = 1200.f;

// SEA
const int ITER_GEOMETRY = 3;
const int ITER_FRAGMENT = 5;
const float SEA_HEIGHT = 0.6;
const float SEA_CHOPPY = 4.0;
const float SEA_SPEED = 0.8;
const float SEA_FREQ = 0.16;
const vec3 SEA_BASE = vec3(0.1, 0.19, 0.22);
const vec3 SEA_WATER_COLOR = vec3(0.8,0.9,0.6);
const mat2 octave_m = mat2(1.6, 1.2, -1.2, 1.6);

// For noise
const mat3 mt = mat3(0.33338, 0.56034, -0.71817,
                     -0.87887, 0.32651, -0.15323,
                     0.15162, 0.69596, 0.61339) * 1.93;

const mat2 m2 = mat2(  0.80,  0.60,
                      -0.60,  0.80 );
const mat2 m2i = mat2( 0.80, -0.60,
                       0.60,  0.80 );
const mat3 m3  = mat3( 0.00,  0.80,  0.60,
                      -0.80,  0.36, -0.48,
                      -0.60, -0.48,  0.64 );
const mat3 m3i = mat3( 0.00, -0.80, -0.60,
                       0.80,  0.36, -0.48,
                       0.60, -0.48,  0.64 );

int FRAME;
float SPEED;
const int SPEED_SCALE = 3;
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
    // Custom id
    int customId;
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
    // Custom id
    int customId;
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
    vec4 fragColor;
    float d;
    int customId;
    bool isEnv;
    bool isAL;
};

// =========== Uniforms ============
// Screen/Camera
uniform vec4 eyePosition;
uniform vec2 screenDimensions;
uniform float initialFar;
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
uniform sampler2D LTC1;
uniform sampler2D LTC2;
uniform sampler2D noise;
uniform sampler2D bluenoise;

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
uniform int numOctaves;
uniform float terrainHeight = 0.f;
uniform float terrainScale;

// ============ CUSTOM SCENE CONFIG =================
// Cannot think about a better way :(

RayMarchObject customObjects[2];

// ================== Utility =======================
vec3 tri(vec3 x) {
    return abs(x-floor(x)-.5);
}

// Transforms p along axis by angle
vec3 rotateAxis(vec3 p, vec3 axis, float angle) {
    return mix(dot(axis, p)*axis, p, cos(angle)) + cross(axis,p)*sin(angle);
}

mat2 rotate2D(float a) {
  float s = sin(a);
  float c = cos(a);
  return mat2(c, -s, s, c);
}

// Applies transformation
vec3 Transform(in vec3 p)
{
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

// Simple hash function
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453);
}

// Function to generate a random 2D vector based on a seed
vec2 random2(vec2 seed) {
    return vec2(hash(seed), hash(seed + vec2(1.0)));
}

// For Shadow effects with area lights
vec3 samplePointOnRectangleAreaLight(vec3 lightPos1, vec3 lightPos2, vec3 lightPos3, vec3 lightPos4, vec2 randomUV) {
    // Calculate vectors defining the rectangle
    vec3 side1 = lightPos2 - lightPos1;
    vec3 side2 = lightPos4 - lightPos1;

    // Use barycentric coordinates to interpolate a point on the rectangle
    vec3 randomPoint = lightPos1 + randomUV.x * side1 + randomUV.y * side2;

    return randomPoint;
}

// Compute the angular falloff term
float angularFalloffFactor(float angle, float innerA, float outerA) {
    float t = (angle - innerA) / (outerA - innerA);
    return -2 * pow(t, 3) + 3 * pow(t, 2);
}

// Gets the attenuation factor
float attenuationFactor(float d, vec3 func) {
    return min(1.f / (func[0] + (d * func[1]) + (d * d * func[2])), 1.f);
}

// Gets the angular falloff term given light direction
float angularFalloff(vec3 L, int i) {
    float cosalpha = dot(-normalize(lights[i].lightDir), L);
    float inner = lights[i].lightAngle - lights[i].lightPenumbra;
    if (cosalpha <= cos(lights[i].lightAngle)){
        return 0.f;
    } else if (cosalpha > cos(inner)) {
        return 1.f;
    } else {
        return 1.f -
                angularFalloffFactor(acos(cosalpha), inner, lights[i].lightAngle);
     }
}


// ============ NOISE =============

// Used in noised (vec3)
float hash1( float n )
{
    return fract( n*17.0*fract( n*0.3183099 ) );
}

// Used in noiseT
float hash1( vec2 p )
{
    p  = 50.0*fract( p*0.3183099 );
    return fract( p.x*p.y*(p.x+p.y) );
}

// Used in fbm_9
float noiseT( in vec2 x )
{
    vec2 p = floor(x);
    vec2 w = fract(x);
    vec2 u = w*w*w*(w*(w*6.0-15.0)+10.0);
    float a = hash1(p+vec2(0,0));
    float b = hash1(p+vec2(1,0));
    float c = hash1(p+vec2(0,1));
    float d = hash1(p+vec2(1,1));
    return -1.0+2.0*(a + (b-a)*u.x + (c-a)*u.y + (a - b - c + d)*u.x*u.y);
}

float noiseW(in vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);

    vec2 u = f * f * (3.0 - 2.0 * f);

    float a = hash(i + vec2(0.0,0.0));
    float b = hash(i + vec2(1.0,0.0));
    float c = hash(i + vec2(0.0,1.0));
    float d = hash(i + vec2(1.0,1.0));

    float result = mix(mix(a, b, u.x),
                        mix(c, d, u.x), u.y);

    return (2.0 * result) - 1.0;
}

// Used in fbmd_8
// value noise, and its analytical derivatives
vec4 noised( in vec3 x )
{
    vec3 p = floor(x);
    vec3 w = fract(x);
    #if 1
    vec3 u = w*w*w*(w*(w*6.0-15.0)+10.0);
    vec3 du = 30.0*w*w*(w*(w-2.0)+1.0);
    #else
    vec3 u = w*w*(3.0-2.0*w);
    vec3 du = 6.0*w*(1.0-w);
    #endif

    float n = p.x + 317.0*p.y + 157.0*p.z;

    float a = hash1(n+0.0);
    float b = hash1(n+1.0);
    float c = hash1(n+317.0);
    float d = hash1(n+318.0);
    float e = hash1(n+157.0);
    float f = hash1(n+158.0);
    float g = hash1(n+474.0);
    float h = hash1(n+475.0);

    float k0 =   a;
    float k1 =   b - a;
    float k2 =   c - a;
    float k3 =   e - a;
    float k4 =   a - b - c + d;
    float k5 =   a - c - e + g;
    float k6 =   a - b - e + f;
    float k7 = - a + b + c - d + e - f - g + h;

    return vec4( -1.0+2.0*(k0 + k1*u.x + k2*u.y + k3*u.z + k4*u.x*u.y + k5*u.y*u.z + k6*u.z*u.x + k7*u.x*u.y*u.z),
                      2.0* du * vec3( k1 + k4*u.y + k6*u.z + k7*u.y*u.z,
                                      k2 + k5*u.z + k4*u.x + k7*u.z*u.x,
                                      k3 + k6*u.x + k5*u.y + k7*u.x*u.y ) );
}

// Used in fbmd_9
// Derivative based noise
// ref: https://iquilezles.org/articles/morenoise/
vec3 noised(vec2 x) {
  vec2 f = fract(x);
  vec2 u = f * f* (3.0 - 2.0 * f);
  vec2 du = 6.0*f*(1.0-f);

  vec2 p = floor(x);
  float a = textureLod(noise, (p+vec2(0.5,0.5)) /256.,0.).x;
  float b = textureLod(noise, (p+vec2(1.5,0.5)) /256.,0.).x;
  float c = textureLod(noise, (p+vec2(0.5,1.5)) /256.,0.).x;
  float d = textureLod(noise, (p+vec2(1.5,1.5)) /256.,0.).x;

  float noiseVal = a+(b-a)*u.x+(c-a)*u.y+(a-b-c+d)*u.x*u.y;
  vec2 noiseDerivative = du*(vec2(b-a,c-a)+(a-b-c+d)*u.yx);

  return vec3(noiseVal, noiseDerivative);
}

// Used in fbm_4
float noiseV(vec3 x ) {
  vec3 p = floor(x);
  vec3 f = fract(x);
  f = f*f*(3.0-2.0*f);
  vec2 uv = (p.xy+vec2(37.0,239.0)*p.z) + f.xy;
  vec2 rg = textureLod(noise,(uv+0.5)/256.0,0.0).yx;
  return mix( rg.x, rg.y, f.z )*2.0-1.0;
}

// Taken from Inigo Quilez's Rainforest ShaderToy:
// https://www.shadertoy.com/view/4ttSWf
float fbm_4( in vec3 x )
{
    float f = 2.0;
    float s = 0.5;
    float a = 0.0;
    float b = 0.5;
    for( int i=min(0, FRAME); i<4; i++ )
    {
        float n = noiseV(x);
        a += b*n;
        b *= s;
        x = f*m3*x;
    }
        return a;
}

// Used in sdTerrain to get noise
float fbm_9( in vec2 x )
{
    float f = 1.9;
    float s = 0.55;
    float a = 0.0;
    float b = 0.5;
    for( int i=0; i<9; i++ )
    {
        float n = noiseT(x);
        a += b*n;
        b *= s;
        x = f*m2*x;
    }

        return a;
}

// Used in cloudFbm
vec4 fbmd_8( in vec3 x )
{
    float f = 2.0;
    float s = 0.65;
    float a = 0.0;
    float b = 0.5;
    vec3  d = vec3(0.0);
    mat3  m = mat3(1.0,0.0,0.0,
                   0.0,1.0,0.0,
                   0.0,0.0,1.0);
    for( int i=0; i<8; i++ )
    {
        vec4 n = noised(x);
        a += b*n.x;          // accumulate values
        if( i<4 )
        d += b*m*n.yzw;      // accumulate derivatives
        b *= s;
        x = f*m3*x;
        m = f*m3i*m;
    }
        return vec4( a, d );
}

// Used in terrainMapD
vec3 fbmd_9( in vec2 x )
{
    float f = 1.9;
    float s = 0.55;
    float a = 0.0;
    float b = 0.5;
    vec2  d = vec2(0.0);
    mat2  m = mat2(1.0,0.0,0.0,1.0);
    for( int i=0; i<9; i++ )
    {
        vec3 n = noised(x);
        a += b*n.x;          // accumulate values
        d += b*m*n.yz;       // accumulate derivatives
        b *= s;
        x = f*m2*x;
        m = f*m2i*m;
    }
    return vec3( a, d );
}

// return smoothstep and its derivative
vec2 smoothstepd( float a, float b, float x)
{
    if( x<a ) return vec2( 0.0, 0.0 );
    if( x>b ) return vec2( 1.0, 0.0 );
    float ir = 1.0/(b-a);
    x = (x-a)*ir;
    return vec2( x*x*(3.0-2.0*x), 6.0*x*(1.0-x)*ir );
}
// ================ SDF Combo Functs ====================
float sdSmoothUnion( float d1, float d2, float k )
{
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h);
}

float sdOnion( in float d, in float h )
{
    return abs(d)-h;
}

// polynomial smooth min (k = 0.1);
float smin( float a, float b, float k )
{
  float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
  return mix( b, a, h ) - k*h*(1.0-h);
}

// ============ Signed Distance Fields ==============
// Define SDF for different shapes here
// - Based on https://iquilezles.org/articles/distfunctions/

vec2 sdTerrain(vec2 p)
{
    float e = fbm_9( p/2000.0 + vec2(1.0,-2.0) );
    float a = 1.0-smoothstep( 0.12, 0.13, abs(e+0.12) ); // flag high-slope areas (-0.25, 0.0)
    e = 600.0*e + 600.0;

    // cliff
    e += 90.0*smoothstep( 552.0, 594.0, e );

    return vec2(e,a);
}

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

// Sierpinski Signed Distance Field
// @param p Point in object space
// @param power (typically 8)
float sdSierpinski(vec3 p)
{
    const int Iterations = 14;
    const float Scale = 1.85;
    const float Offset = 2.0;
    vec3 a1 = vec3(1,1,1);
    vec3 a2 = vec3(-1,-1,1);
    vec3 a3 = vec3(1,-1,-1);
    vec3 a4 = vec3(-1,1,-1);
    vec3 c;
    float dist, d;

    for (int n = 0; n < Iterations; n++) {
        if(p.x+p.y<0.) p.xy = -p.yx; // fold 1
        if(p.x+p.z<0.) p.xz = -p.zx; // fold 2
        if(p.y+p.z<0.) p.zy = -p.yz; // fold 3
        p = p*Scale - Offset*(Scale-1.0);
    }

    return length(p) * pow(Scale, -float(Iterations));
}

// Sphere Signed Distance Field
// @param p Point in object space
// @param r Radius
float sdSphere(vec3 p, float r)
{
  return length(p)-r;
}

float sdSine(vec3 p) {
  return 1.0 - (sin(p.x) + sin(p.y) + sin(p.z))/3.0;
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

float sdCapsule( vec3 p, vec3 a, vec3 b, float r )
{
  vec3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
  return length( pa - ba*h ) - r;
}

float sdRock(in vec3 p) {
    float d = length(p.xy-vec2(-4.,0.))-2.;
    d = smin(d, sdCapsule(p, vec3(-5.,0.,7.), vec3(0.,2.,7.), 1.), 1.7);
    d = smin(d, p.y-.1+smoothstep(-3.1,5.,p.x)*2., 0.1);

    // Add detail only if we are closed of the surface
    if (d < 1.) {
        // Inspired by many shaders of Shane, the master of the rock.
        float z = 1.;
        for(int i = 0; i < 6; i++)
        {
            d -= dot(tri(p*.5 + tri(p.yzx*.375)), vec3(.4*z));
            z *= -0.55;
            p = p*mt;
        }
    }

    return d;
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

// Menger Sponge Signed Distance Field
// Great ref: https://www.youtube.com/watch?v=6IWXkV82oyY&t=1502s
// @param p Point in object space
// @param power (typically 8)
float sdMengerSponge(vec3 p, out vec4 res)
{

    float d = sdBox(p,vec3(1));
    res = vec4( d, 1.0, 0.0, 0.0 );
    // float ani = smoothstep( -0.2, 0.2, -cos(0.5*iTime) );
    // float off = 1.5*sin( 0.01*iTime );
    float s = 1.0;

    for(int m=0; m<4; m++) {
        // p = mix( p, ma*(p+off), ani );
        vec3 a = mod( p*s, 2.0 )-1.0;
        s *= 3.0;
        vec3 r = abs(1.0 - 3.0*abs(a));
        float da = max(r.x,r.y);
        float db = max(r.y,r.z);
        float dc = max(r.z,r.x);
        float c = (min(da,min(db,dc))-1.0)/s;
        if(c > d) {
            d = c;
            res = vec4( d, min(res.y,0.2*da*db*dc), (1.0+float(m))/4.0, 0.0 );
        }
    }
    return d;
}

float sdPlane( vec3 p, vec3 n, float h )
{
  // n must be normalized
  return dot(p,n) + h;
}

//const float NOISE_PROPORTION = 0.25;
//const float NOISE_FREQUENCY = 20.;
//const float NOISE_GRADIENT_MAGNITUDE = (1. * NOISE_FREQUENCY * 1.32);

float sdColumn( vec3 p )
{
    vec3 bp1 = p; vec3 bp2 = p; vec3 cp = p;
    float bp1_scale = mix(1.5, 2.5, smoothstep(0, 0.5, p.y));
    float bp2_scale = mix(2.5, 1.5, smoothstep(6.5, 7., p.y));

    // pillar base
    bp1.xz *= bp1_scale;
    float baseBox = sdBox(bp1, vec3(0.75, 0.50, 0.75)) / bp1_scale; // [-0.5, 0.5]

    // pillar core
    float coreCyl = sdCylinder(cp + vec3(0, -3.5, 0), 3, 0.2); // [0.5, 6.5]
    cp.xz *= rotate2D(cp.y);
    float bbcore = sdBox(cp + vec3(0, -3.5, 0), vec3(0.25, 2, 0.25));
    float pillarCore = sdSmoothUnion(coreCyl, bbcore, 0.9);


    // pillar top
    bp2.xz *= bp2_scale;
    float topBox = sdBox(bp2 + vec3(0, -7, 0), vec3(0.75, 0.50, 0.75)) / bp2_scale; // [6.5, 7.5]

    float dt = sdSmoothUnion(baseBox, pillarCore, 0.4);
    dt = sdSmoothUnion(dt, topBox, 0.4);
    return dt;
}

float sdBalls(vec3 p) {
    float ballRadius = 1;
    const float MAX_DIST = 100.0;
    float t = iTime / 3.0 + 10500.0;
    float balls = MAX_DIST;
    for (float i = 1.0; i < 4.0; i += 1.3) {
      for (float j = 1.0; j < 4.0; j += 1.3) {
        float cost = cos(t * j);
        balls = smin(balls, sdSphere(p + vec3(sin(t * i) * j, cost * i, cost * j), ballRadius), 0.7);
      }
    }
    return balls;
}

float sdCUSTOM(vec3 p, out int customId) {
    // Custom ID needed for applying different material
    float dt = 0.f;
    float bb = sdBox(p, vec3(5.));
    return bb;
// === BALL & PILLAR Scene ===
//    float dt = sdBalls(p - vec3(0, 4.75f, 0));
//    customId = 0;

//    float col_scale = 2.;
//    p /= col_scale;
//    float c1 = sdColumn(p + vec3(3, 0, -5));
//    float c2 = sdColumn(p + vec3(5, 0, 0));
//    float c3 = sdColumn(p + vec3(3, 0, 5));
//    float c4 = sdColumn(p + vec3(-3, 0, 5));
//    float c5 = sdColumn(p + vec3(-5, 0, 0));
//    float c6 = sdColumn(p + vec3(-3, 0, -5));
//    float c = min(c1, min(c2, min(c3, min(c4, min(c5, c6)))));

//    if (c < dt) {
//        customId = 1;
//        dt = c;
//    }

    return dt;
}

float sdFlowerBall(vec3 p) {
    vec2 t = vec2(1.5, 1.5 * .2);

    float s = sdTorus(p, t);

    p = rotateAxis(p, vec3(0, 0, 1), 90);

    float s2 = sdTorus(p, t);

    p = rotateAxis(p, vec3(0, 0, 1), 90);

    float s3 = sdTorus(p, t);

    float s4 = sdSphere(p, 1.5);

    return max(s4, min(min(s, s2), s3));
}

// Given a point in object space and type of the SDF
// Invoke the appropriate SDF function and return the distance
// @param p Point in object space
// @param type Type of the object
float sdMatch(vec3 p, int type, int id, out int customId, out vec4 trapCol)
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
        return sdBox(p, vec3(0.5, 0.5, 0));
    } else if (type == MANDELBROT) {
        return sdMandelBrot(vec2(p));
    } else if (type == MANDELBULB) {
        return sdMandelBulb(p, trapCol);
    } else if (type == MENGERSPONGE) {
        return sdMengerSponge(p, trapCol);
    } else if (type == SIERPINSKI) {
        return sdSierpinski(p);
    } else if (type == CUSTOM) {
        return sdCUSTOM(p, customId);
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
    int minObj = -1; int minCId;
    int customId;
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
        currD = sdMatch(po, obj.type, i, customId, trapCol) * obj.scaleFactor;
        if (currD < minD) {
            // Update if we found a closer object
            minD = currD;
            minObj = i;
            minCId = customId;
        }
    }
    // Populate the struct
    SceneMin res;
    res.minD = minD;
    res.minObjIdx = minObj;
    res.customId = minCId;
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
  closest.minD = 1000000;
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
      res.d = rayDepth - closest.minD;
      res.trap = closest.trap; res.customId = closest.customId;
  } else {
      // NO HIT
      res.intersectObj = -1;
  }
  return res;
}

// =============== SUN & SKY =================

// [0, 1]
float timeOfDay = mod(iTime, 24.) / 24.0;
float sunriseStart = 0.2;
float sunsetStart = 0.8;

// Get current sun direction
vec3 getSunDir() {
    float elevationAngle = mix(0, 3.14, timeOfDay);
    return normalize(vec3(cos(elevationAngle), sin(elevationAngle), -0.577));
}

// Get current sky color
vec3 getSkyColor() {
    vec3 dayColor = vec3(0.8, 0.9, 1.1);
    vec3 sunriseColor = vec3(1.0, 0.5, 0.2);
    vec3 sunsetColor = vec3(1.0, 0.8, 0.5);

    vec3 blendedColor = mix(sunriseColor, dayColor, smoothstep(0.0, sunriseStart, timeOfDay));
    blendedColor = mix(blendedColor, sunsetColor, smoothstep(sunsetStart, 1.0, timeOfDay));

    return blendedColor;
}

// Get current sun color
vec3 getSunColor() {
    vec3 sunriseColor = vec3(1.0, 0.5, 0.2);
    vec3 daytimeColor = vec3(1.f, 1.f, 0.8f);
    vec3 sunsetColor = vec3(1.0, 0.8, 0.5);

    vec3 sunColor = mix(sunriseColor, daytimeColor, smoothstep(0.0, sunriseStart, timeOfDay));
    sunColor = mix(sunColor, sunsetColor, smoothstep(sunsetStart, 1.f, timeOfDay));
    return sunColor;
}

// Get the background color
vec3 getSky(vec3 rd) {
    vec3 col  = getSkyColor()*(0.6+0.4*rd.y);
    col += getSunColor() *
            pow(clamp(
                    dot(rd, getSunDir()),
                    0.0,1.0),
                32.0 );
    return col;
}

// ================== Phong Total Illumination =================

// Computes the shadow scale for soft shadow
// - https://iquilezles.org/articles/rmshadows/
// @param ro Ray origin
// @param rd Ray direction
// @param mint Starting t
// @param maxt End t
// @param k How "hard" we want the shadow to be
// @retunrs Result of raymarching
RayMarchRes softshadow(vec3 ro, vec3 rd, float mint, float maxt, float k ) {
    float res = 1.0;
    float rayDepth = mint;
    RayMarchRes r;
    SceneMin closest;
    for(int i=0; i < MAX_STEPS; i++) {
        closest = sdScene(ro + rd*rayDepth);
        if(abs(closest.minD) < SURFACE_DIST || rayDepth > maxt) break;
        res = min(res, k * closest.minD/(rayDepth));
        // March the ray
        rayDepth += abs(closest.minD);
    }
    if (abs(closest.minD) < SURFACE_DIST) {
        // HIT
        r.intersectObj = closest.minObjIdx;
        r.d = res;
        r.customId = closest.customId;
    } else {
        // NO HIT
        r.intersectObj = -1;
    }
    return r;
}

// Calculate the ambient occlusion
// https://iquilezles.org/articles/nvscene2008/rwwtt.pdf
float calcAO(in vec3 pos, in vec3 nor) {
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
vec3 getDiffuse(vec3 p, vec3 n, bool custom, int type,
                vec3 cD, int texLoc, mat4 invModel,
                float rU, float rV, float blend) {
    if (texLoc == -1) {
        // No texture used
        return kd * cD;
    }
    // Texture used -> find uv
    vec2 uv;
    // (Note) For global transformation
    // vec3 po = vec3(obj.invModelMatrix * vec4(Transform(p), 1.f));
    vec3 po = vec3(invModel * vec4(p, 1.f));
    if (type == CUBE) {
        uv = uvMapCube(po, rU, rV);
    } else if (type == CONE) {
        uv = uvMapCone(po, rU, rV);
    } else if (type == CYLINDER) {
        uv = uvMapCylinder(po, rU, rV);
    } else if (type == SPHERE) {
        uv = uvMapSphere(po, rU, rV);
    } else {
        // Tri-planar
        vec3 colXZ = texture(objTextures[texLoc], p.xz*.5 + .5).rgb;
        vec3 colYZ = texture(objTextures[texLoc], p.yz*.5 + .5).rgb;
        vec3 colXY = texture(objTextures[texLoc], p.xy*.5 + .5).rgb;

        n = abs(n);
        n *= pow(n, vec3(10));
        n /= n.x + n.y + n.z;

        vec3 col = colYZ * n.x + colXZ * n.y + colXY * n.z;
        return (1.f - blend) * kd * cD + blend * col;
    }
    // Sample
    vec4 texVal = texture(objTextures[texLoc], uv);
    // Linear interpolate
    return (1.f - blend) * kd * cD + blend * vec3(texVal);
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

// Get Area Light
vec3 getAreaLight(vec3 N, vec3 V, vec3 P, int lightIdx, vec3 cD,
                  vec3 cS, bool custom, int type, int texLoc, mat4 invModel,
                  float rU, float rV, float blend) {
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
    // Evaluate LTC shading
    vec3 diffuse = LTC_Evaluate(N, V, P, mat3(1), areaLight.points, areaLight.twoSided);
    vec3 specular = LTC_Evaluate(N, V, P, Minv, areaLight.points, areaLight.twoSided);
    // GGX BRDF shadowing and Fresnel
    specular *= cS*t2.x + (areaLight.intensity - cS) * t2.y;
    vec3 col = areaLight.lightColor * 1.0
            * (specular + getDiffuse(P, N, custom, type, cD, texLoc, invModel, rU, rV, blend)
               * diffuse);
    return col;
}

void setCustomMat(int customId, out vec3 a, out vec3 d, out vec3 s,
                  out int texLoc, out float rU, out float rV, out float blend,
                  out int type, out float shininess) {

// BALL & Pillar Scene Material
//    if (customId == 0) {
//        // ball
//        a = vec3(0.3); d = vec3(1.0, 0.8, 0.6); s = vec3(0.3);
//        texLoc = -1; rU = 2.; rV = 2.;
//        type = CUSTOM; blend = 1.f; shininess = 0.4;
//    } else if (customId == 1) {
//        // pillar
//        a = vec3(0.1); d = vec3(0.5); s = vec3(0.);
//        texLoc = 0; rU = 2.; rV = 2.;
//        type = CUSTOM; blend = 0.6f; shininess = 0.;
//    }
}

// Gets Phong Light
// @param N normal
// @param intersectObj Id of the intersected object
// @param p Intersection point
// @param rd Ray direction
// @returns phong color for that fragment
vec3 getPhong(vec3 N, int intersectObj, vec3 p, vec3 ro, vec3 rd, float far, bool custom) {
    vec3 total = vec3(0.f); RayMarchObject obj = objects[intersectObj];
    // Get material
    vec3 cAmbient = obj.cAmbient,
         cDiffuse = obj.cDiffuse,
         cSpecular = obj.cSpecular;
    float rU = obj.repeatU, rV = obj.repeatV; int texLoc = obj.texLoc; int type = obj.type;
    mat4 invModel = obj.invModelMatrix; float blend = obj.blend; float shininess = obj.shininess;
    if (custom) setCustomMat(intersectObj, cAmbient, cDiffuse, cSpecular,
                             texLoc, rU, rV, blend,
                             type, shininess);

    // Ambience
    float ao = 1.f;
    if (enableAmbientOcculusion) ao = calcAO(p, N);
    total += cAmbient * ka * ao;

    // Loop Lights
    for (int i = 0; i < numLights; i++) {
        float fAtt = 1.f; float aFall = 1.f; LightSource li = lights[i];
        float d = length(p - li.lightPos);
        vec3 currColor = vec3(0.f); vec3 L; float maxT;
        if (li.type == POINT) {
            L = normalize(li.lightPos - p);
            fAtt = attenuationFactor(d, li.lightFunc);
            maxT = length(li.lightPos - p);
        } else if (li.type == DIRECTIONAL) {
            L = normalize(-li.lightDir);
            L = getSunDir();
            maxT = far;
        } else if (li.type == SPOT) {
            L = normalize(li.lightPos - p);
            fAtt = attenuationFactor(d, li.lightFunc);
            maxT = length(li.lightPos - p);
            aFall = angularFalloff(L, i);
        }

        vec3 V = normalize(-rd);
        // Area Light Calculation
        if (li.type == AREA) {
            vec3 areaColor = vec3(0.f);
            vec3 p1 = li.points[0], p2 = li.points[1], p3 = li.points[2], p4 = li.points[3];
            for (int idx = 0; idx < AREA_LIGHT_SAMPLES; idx++) {
                // Sample a point and cast a shadow ray towards it
                vec3 randomP = samplePointOnRectangleAreaLight(p1,p2,p3,p4,vec2(rd + idx));
                L = normalize(randomP - p);
                float NdotL = dot(N, L);
                if (NdotL <= 0.005f) continue;
                maxT = length(randomP - p);
                // Check for shadow
                RayMarchRes res = softshadow(p + N * SURFACE_DIST * 5.f, L, 0, maxT, 8);
                if (res.intersectObj != -1) {
                    // Shadow Ray intersected an object
                    // We need to check if the intersected object
                    // is indeed area light or not
                    if (objects[res.intersectObj].lightIdx != i) continue;
                }
                // calculate light contribution
                areaColor += getAreaLight(N, V, p, i, cDiffuse, cSpecular, custom, type, texLoc, invModel, rU, rV, blend);
            }
            currColor += areaColor / AREA_LIGHT_SAMPLES;
        } else {
            // Shadow
            RayMarchRes res = softshadow(p + N * SURFACE_DIST * 5.f, L, 0, maxT, 8);
            if (res.intersectObj != -1) continue; // shadow ray intersect
            // Diffuse
            float NdotL = dot(N, L);
            if (NdotL <= 0.005f) continue; // pointing away
            NdotL = clamp(NdotL, 0.f, 1.f);
            currColor +=  getDiffuse(p, N, custom, type, cDiffuse, texLoc, invModel, rU, rV, blend)
                    * NdotL
                    // * li.lightColor;
                    * getSunColor();

            // Specular
            vec3 R = reflect(-L, N);
            float RdotV = clamp(dot(R, V), 0.f, 1.f);
            currColor += getSpecular(RdotV, cSpecular, shininess)
                    //* li.lightColor;
                    * getSunColor();
            // Add the light source's contribution
            currColor *= fAtt * aFall;
            if (enableSoftShadow) currColor *= res.d;
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

// ============== CLOUDS ================

vec3 fog( in vec3 col, float t ) {
    vec3 ext = exp2(-t*0.00025*vec3(1,1.5,4));
    return col*ext + (1.0-ext)*vec3(0.55,0.55,0.58); // 0.55
}

vec4 cloudsFbm( in vec3 pos ) {
    return fbmd_8(pos*0.0015+vec3(2.0,1.1,1.0)+0.07*vec3(iTime,0.5*iTime,-0.15*iTime));
}

float cloudsShadowFlat( in vec3 ro, in vec3 rd ) {
    float t = (CLOUD_MID-ro.y)/rd.y;
    if( t<0.0 ) return 1.0;
    vec3 pos = ro + rd*t;
    return cloudsFbm(pos).x;
}

vec4 cloudsMap( in vec3 pos, out float nnd ) {
    float d = abs(pos.y-CLOUD_MID)-4.f;
    vec3 gra = vec3(0.0,sign(pos.y-CLOUD_MID),0.0);

    vec4 n = cloudsFbm(pos);
    d += 400.0*n.x * (0.7+0.3*gra.y);

    if( d>0.0 ) return vec4(-d,0.0,0.0,0.0);

    nnd = -d;
    d = min(-d/100.0,0.25);

    return vec4( d, gra );
}

bool cloudMarch(int steps, in vec3 ro, in vec3 rd, in float minT, in float maxT,
                inout vec4 sum) {
    bool hasHit = false;
    float stepSize = CLOUD_STEP_SIZE;
    float opaqueVisibility = 1.f;
    float t = minT;
    float lastT = -1.0;
    float thickness = 0.0;
    vec3 sunColor = getSunColor();
    for (int i = 0; i < steps; i++) {
        vec3 pos = ro + rd * t;
        float nnd;
        vec4 denGra = cloudsMap(pos, nnd);
        float den = denGra.x;
        float dt = max(CLOUD_STEP_SIZE, 0.011 * t);
        if (den > 0.001) {
            hasHit = true;
            float kk;
            cloudsMap( pos+getSunDir()*70.0, kk );
            float sha = 1.0-smoothstep(-200.0,200.0,kk); sha *= 1.5;

            vec3 nor = normalize(denGra.yzw);
            float dif = clamp( 0.4+0.6*dot(nor,getSunDir()), 0.0, 1.0 )*sha;
            float fre = clamp( 1.0+dot(nor,rd), 0.0, 1.0 )*sha;
            float occ = 0.2+0.7*max(1.0-kk/200.0,0.0) + 0.1*(1.0-den);
            // lighting
            vec3 lin  = vec3(0.0);
                 lin += vec3(0.70,0.80,1.00)*1.0*(0.5+0.5*nor.y)*occ;
                 lin += vec3(0.10,0.40,0.20)*1.0*(0.5-0.5*nor.y)*occ;
                 lin += sunColor *3.0*dif*occ + 0.1;
            // color
            vec3 col = vec3(0.8,0.8,0.8)*0.45;
            col *= lin;
            col = fog( col, t );
            // front to back blending
            float alp = clamp(den*0.5*0.125*dt,0.0,1.0);
            col.rgb *= alp;
            sum = sum + vec4(col,alp)*(1.0-sum.a);
            thickness += dt*den;
            if( lastT < 0.0 ) lastT = t;

        } else {
            dt = abs(den) + 0.2;
        }
        t += dt;
        if (sum.a > 0.995 || t > maxT) break;
    }
    sum.xyz += max(0.0, 1.0 - 0.0125 * thickness) * sunColor
            * 0.3 * pow(clamp(dot(getSunDir(), rd), 0.0, 1.0), 32.0);
    return hasHit;
}

// Performs raymarching for volumetric data
// To prevent banding from happening, offset the ray start position using
// blue noise texture (aka blue noise dithering)
vec4 raymarchVolumetric(vec3 ro, vec3 rd, inout bool hit,
                        in float minT, in float maxT) {
    vec4 sum = vec4(0.0);
    // get noise
    float blueNoise = texture(bluenoise, gl_FragCoord.xy / 1024.0).r;
    float off = float(FRAME%64) + 0.61803398875f;
    // different starting points
    minT += CLOUD_STEP_SIZE * fract(off + blueNoise);
    // march towards clouds
    hit = cloudMarch(128, ro, rd, minT, maxT, sum);
    return clamp( sum, 0.0, 1.0 );
}

// Function that renders volumetric cloud
vec3 cloudRender( in vec3 ro, in vec3 rd, in vec3 bgCol, out bool hit, in float maxT ) {
    vec3 col = bgCol; float minT = 0;
    // Raymarch volumetric cloud
    // Bounding Volume
    float tl = ( CLOUD_LOW-ro.y)/rd.y;
    float th = ( CLOUD_HIGH-ro.y)/rd.y;
    if( tl>0.0 ) { minT = max( minT, tl ); } else { hit = false; return col; }
    if( th>0.0 ) maxT = min( maxT, th );
    vec4 res = raymarchVolumetric(ro, rd, hit, minT, maxT);
    // Blend with background color
    col = col*(1.0-res.w) + res.xyz;
    return col;
}

// ================== Terrain ====================
float raymarchTerrain( in vec3 ro, in vec3 rd, float tmin, float tmax ) {
    // bounding plane
    float tp = (TERRAIN_HIGH-ro.y)/rd.y;
    if( tp>0.0 ) tmax = min( tmax, tp );

    // raymarch
    float dis, th;
    float t2 = -1.0;
    float t = tmin;
    float ot = t;
    float odis = 0.0;
    float odis2 = 0.0;
    for( int i=0; i<400; i++ )
    {
        th = 0.001*t;
        vec3  pos = ro + t*rd;
        vec2  env = sdTerrain( pos.xz );
        float hei = env.x;
        // terrain
        dis = pos.y - hei;
        if( dis<th ) break;
        ot = t;
        odis = dis;
        t += dis*0.8*(1.0-0.75*env.y); // slow down in step areas
        if( t>tmax ) break;
    }

    if( t>tmax ) t = -1.0;
    else t = ot + (th-odis)*(t-ot)/(dis-odis); // linear interpolation for better accuracy
    return t;
}

vec4 terrainMapD( in vec2 p ) {
    vec3 e = fbmd_9( p/2000.0 + vec2(1.0,-2.0) );
    e.x  = 600.0*e.x + 600.0;
    e.yz = 600.0*e.yz;

    // cliff
    vec2 c = smoothstepd( 550.0, 600.0, e.x );
        e.x  = e.x  + 90.0*c.x;
        e.yz = e.yz + 90.0*c.y*e.yz;     // chain rule

    e.yz /= 2000.0;
    return vec4( e.x, normalize( vec3(-e.y,1.0,-e.z) ) );
}

vec3 terrainNormal( in vec2 pos ) {
    vec2 e = vec2(0.03,0.0);
    return normalize(vec3(sdTerrain(pos-e.xy).x - sdTerrain(pos+e.xy).x,
                        2.0*e.x,
                        sdTerrain(pos-e.yx).x - sdTerrain(pos+e.yx).x ) );
}

float terrainShadow( in vec3 ro, in vec3 rd, in float mint ) {
    float res = 1.0;
    float t = mint;
    for( int i=0; i<32; i++ ) {
        vec3  pos = ro + t*rd;
        vec2  env = sdTerrain( pos.xz );
        float hei = pos.y - env.x;
        res = min( res, 32.0*hei/t );
        if( res<0.0001 || pos.y>TERRAIN_HIGH ) break;
        t += clamp( hei, 2.0+t*0.1, 100.0 );
    }
    return clamp( res, 0.0, 1.0 );
}

// Mostly taken from Inigo's work :)
RenderInfo terrainRender(in vec3 ro, in vec3 rd, inout bool hit, in float maxT, in vec3 bgCol) {
    RenderInfo ri; vec3 col = bgCol; ri.d = maxT;
    // Raymarch terrain
    float res = raymarchTerrain(ro, rd, 15.f, maxT);
    if (res > 0.0) {
        // If Hit
        hit = true; ri.d = res;
        vec3 p = ro + rd * res; vec3 pn = terrainNormal(p.xz);
        vec3 speC = vec3(1.0); vec3 epos = p + vec3(0.0,4.8,0.0);
        vec3 sunColor = getSunColor();
        float sha1  = terrainShadow( p+vec3(0,0.02,0), getSunDir(), 0.02 );
        sha1 *= smoothstep(-0.325,-0.075,cloudsShadowFlat(epos, getSunDir()));
        // bump map
        vec3 nor = normalize( pn + 0.8*(1.0-abs(pn.y))*0.8*fbmd_8( (p-vec3(0,600,0))*0.15*vec3(1.0,0.2,1.0) ).yzw );
        col = vec3(0.18,0.12,0.10)*.85;
        col = 1.0*mix( col, vec3(0.1,0.1,0.0)*0.2, smoothstep(0.7,0.9,nor.y) );
        float dif = clamp( dot( nor, getSunDir()), 0.0, 1.0 );
        dif *= sha1;
        float bac = clamp( dot(normalize(vec3(-getSunDir().x,0.0,-getSunDir().z)),nor), 0.0, 1.0 );
        float foc = clamp( (p.y/2.0-180.0)/130.0, 0.0,1.0);
        float dom = clamp( 0.5 + 0.5*nor.y, 0.0, 1.0 );

        vec3  lin  = 1.0*0.2*mix(0.1*vec3(0.1,0.2,0.1),sunColor*3.0,dom)*foc;
              lin += 1.0*8.5*sunColor*dif;
              lin += 1.0*0.27*sunColor*bac*foc;
        speC = vec3(4.0)*dif*smoothstep(20.0,0.0,abs(p.y/2.0-310.0)-20.0);
        col *= lin;
    }
    ri.fragColor = vec4(col, 1.f);
    return ri;
}

// =========================== SEA =============================
// Get sea wave octave.
float sea_octave(vec2 uv, float choppy)
{
    uv += noiseW(uv);
    vec2 wv = 1.0 - abs(sin(uv));
    vec2 swv = abs(cos(uv));
    wv = mix(wv, swv, wv);
    return pow(1.0 - pow(wv.x * wv.y, 0.65), choppy);
}

vec3 getSeaColor(vec3 p, vec3 n, vec3 l, vec3 eye, vec3 dist)
{
    float fresnel = clamp(1.0 - dot(n, -eye), 0.0, 1.0);
    fresnel = pow(fresnel, 3.0) * 0.65;

    vec3 reflected = getSky(reflect(eye, n));
    vec3 refracted = SEA_BASE +
            pow(dot(n, l) * 0.4 + 0.6, 80) *
            SEA_WATER_COLOR * 0.12;

    vec3 color = mix(refracted, reflected, fresnel);

    float atten = max(1.0 - dot(dist, dist) * 0.001, 0.0);
    color += SEA_WATER_COLOR * (p.y - SEA_HEIGHT) * 0.18 * atten;

    float nrm = (60. + 8.0) / (PI * 8.0);
    float spec = pow(max(dot(reflect(eye, n), l), 0.), 60.) * nrm;
    color += spec;

    return color;
}

float SEA_TIME = 1. + iTime * SEA_SPEED;

// p is ray position.
float seaMap(vec3 p) {
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    float choppy = SEA_CHOPPY;

    // XZ plane.
    vec2 uv = p.xz;

    float d, h = 0.0;

    // FBM
    for (int i = 0; i < ITER_GEOMETRY; i++) {
        d = sea_octave((uv + SEA_TIME) * freq, choppy);
        d += sea_octave((uv - SEA_TIME) * freq, choppy);
        h += d * amp;
        uv *= octave_m;
        freq *= 2.0;
        amp *= 0.2;
        choppy = mix(choppy, 1.0, 0.2);
    }
    return p.y - h;
}

// p is ray position.
// This function calculate detail map with more iteration count.
float seaMapD(vec3 p) {
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    float choppy = SEA_CHOPPY;

    vec2 uv = p.xz;

    float d, h = 0.0;

    // ITER_FRAGMENT = 5
    for (int i = 0; i < ITER_FRAGMENT; i++) {
        d = sea_octave((uv + SEA_TIME) * freq, choppy);
        d += sea_octave((uv - SEA_TIME) * freq, choppy);
        h += d * amp;
        uv *= octave_m;
        freq *= 2.0;
        amp *= 0.2;
        choppy = mix(choppy, 1.0, 0.2);
    }
    return p.y - h;
}

vec3 getSeaNormal(vec3 p, float eps)
{
    vec3 n;
    n.y = seaMapD(p);
    n.x = seaMapD(vec3(p.x + eps, p.y, p.z)) - n.y;
    n.z = seaMapD(vec3(p.x, p.y, p.z + eps)) - n.y;
    n.y = eps;
    return normalize(n);
}

float seaMapHeight(in vec3 ro, in vec3 rd, out vec3 p, float maxT) {
    float tm = 0.0; float tx = 1000.0;

    // Calculate 1000m far distance map.
    float hx = seaMap(ro + rd * tx);

    // if hx over 0.0 is that ray on the sky
    if(hx > 0.0) { p = vec3(0.0); return tx; }

    float hm = seaMap(ro + rd * tm);
    float tmid = 0.0;

    for (int i = 0; i < 8; i++) {
        // Normalized by max distance
        float f = hm / (hm - hx);

        tmid = mix(tm, tx, f);
        p = ro + rd * tmid;

        if (tmid > maxT) {
            return -1;
        }

        float hmid = seaMap(p);

        if (hmid < 0.0) {
            tx = tmid; hx = hmid;
        } else {
            tm = tmid;  hm = hmid;
        }
    }
    return tmid;
}

RenderInfo seaRender(in vec3 ro, in vec3 rd, inout bool hit, in float maxT, in vec3 bgCol) {
    RenderInfo ri; vec3 col = vec3(0.f); ri.d = maxT; ri.isEnv = true;
    vec3 p;
    // Trace
    float t = seaMapHeight(ro, rd, p, maxT);

    if (length(p) == 0 || t == -1) { ri.fragColor = vec4(bgCol, 1); return;}

    hit = true; ri.d = t;

    // Shade
    vec3 d = p - ro;
    vec3 n = getSeaNormal(p, dot(d, d) * 0.1 / screenDimensions.x);

    // Get Sky
    vec3 s = getSky(rd);
    vec3 sc = getSeaColor(p, n, getSunDir(), rd, d);
    float t2 = pow(smoothstep(0.0, -0.05, rd.y), 0.3);
    vec3 color = mix(s, sc, t2);
    ri.fragColor = vec4(color, 1.);
    return ri;
}


// =============================================================
// Given ray origin and ray direction, performs a raymarching
// @param ro Ray origin
// @param rd Ray direction
// @param i IntersectionInfo we are populating
// @param side Determines if we are inside or outside of an object (for refraction)
RenderInfo render(in vec3 ro, in vec3 rd, out IntersectionInfo i,
                  in float side, in float maxT, in vec3 bgCol) {
    RenderInfo ri; i.intersectObj = -1;
    // Raymarching
    RayMarchRes res = raymarch(ro, rd, maxT, side);
    if (res.intersectObj == -1) {
        // NO HIT
        ri.fragColor = vec4(bgCol, 1.f);
        // If no hit but sky box is used, sample
        if (enableSkyBox) ri.fragColor = vec4(texture(skybox, rd).rgb, 1.f);
        ri.isAL = false; ri.isEnv = true; ri.d = maxT; return ri;
    }

    // HIT
    ri.isEnv = false; ri.d = res.d;
    vec3 p = ro + rd * res.d; vec3 pn = getNormal(p); vec3 col;

    RayMarchObject obj = objects[res.intersectObj];
    if (obj.isEmissive) {
        // Area Light
        col = obj.color; ri.fragColor = vec4(col, 1.f); ri.isAL = true; return ri;
    }

    ri.isAL = false;

    if (obj.type == CUSTOM) {
        col = getPhong(pn, res.customId, p, ro, rd, maxT, true);
    } else if (obj.type == MANDELBULB) {
        // Orbit Trap to color
        col = vec3(0.2);
        col = mix( col, vec3(0.10,0.20,0.30), clamp(res.trap.y,0.0,1.0) );
        col = mix( col, vec3(0.02,0.10,0.30), clamp(res.trap.z*res.trap.z,0.0,1.0) );
        col = mix( col, vec3(0.30,0.10,0.02), clamp(pow(res.trap.w,6.0),0.0,1.0) );
        col *= 0.5;
        col *= getPhong(pn, res.intersectObj, p, ro, rd, maxT, false) * 8;
    } else if (obj.type == MENGERSPONGE) {
        // Orbit Trap to color
        col = 0.5 + 0.5*cos(vec3(0,1,2)+2.0*res.trap.z), 1.f;
        col *= getPhong(pn, res.intersectObj, p, ro, rd, maxT, false);
    } else {
        col = getPhong(pn, res.intersectObj, p, ro, rd, maxT, false);
    }
    // set output variable
    i.p = p; i.n = pn; i.rd = rd; i.intersectObj = res.intersectObj;
    // set return
    ri.fragColor = vec4(col, 1.f); ri.customId = res.customId;
    return ri;
}

vec3 render2D(vec2 pos) {
    float scol = sdMandelBrot(twoDFragCoord.xy);
    return pow( vec3(scol), vec3(0.9,1.1,1.4) );
}

// Set up our scene based on preprocessor directives
void setScene(inout vec3 ro, inout vec3 rd, inout vec3 bgCol, out float far) {
    // === Update Globals ===
    FRAME += 1;
    SPEED = iTime * SPEED_SCALE;
    // === Perspective divide ===
    ro = nearClip.xyz / nearClip.w;
    vec3 farC = farClip.xyz / farClip.w;

#ifdef TERRAIN
    // SCENE #1
    ro.y += 100.f;
#endif
    // SCENE #2
    // ro.y += 8.;

#ifdef SEA
    ro.y += 5;
#endif
    // == NO rotation ==
    rd = normalize(farC - ro);

    // == Smooth zoom in/out ==
//    float disp = smoothstep(-1.,1., sin(iTime));
//    ro.xz += disp;
    // == Rotation about the center ==
//    float rotationAngle = iTime / 7.;
//    ro = rotateAxis(ro, vec3(0.0, 1.0, 0.0), rotationAngle);
//    rd = normalize(farC - ro);
//    rd = rotateAxis(rd, vec3(0.0, 1.0, 0.0), rotationAngle);

#ifdef SKY_BACKGROUND
    bgCol = getSky(rd);
#else
#ifdef WHITE_BACKGROUND
    bgCol = vec3(1.f);
    // SCENE #2
    // bgCol = vec3(texture(objTextures[1], rd.xy * 6)).rgb;
#else
    bgCol = vec3(0.f);
#endif
#endif
    // === Set far plane ===
#ifdef CLOUD
    far = 2000.f;
#else
    far = initialFar;
#endif
}

void main() {
    // === 2D Render ===
    if (isTwoD) { fragColor = vec4(render2D(twoDFragCoord.xy), 1.f); return; }

    // === set scene ===
    vec3 ro, rd, bgCol; float far;
    setScene(ro, rd, bgCol, far);

    vec4 phong, refl = vec4(0.f), refr = vec4(0.f), cres;
    bool cloudHit = false, terrainHit = false, seaHit = false;
    IntersectionInfo info, oi;
    RenderInfo ri, tr, sr;

    // === Main render ===
    ri = render(ro, rd, info, OUTSIDE, far, bgCol);
    sr.d = ri.d; tr.d = ri.d;
    // === Sea render ===
#ifdef SEA
    sr = seaRender(ro, rd, seaHit, ri.d, bgCol);
#endif
    // === Terrain render ===
#ifdef TERRAIN
    tr = terrainRender(ro, rd, terrainHit, sr.d, bgCol);
#endif
    // === Cloud render ===
#ifdef CLOUD
    cres = vec4(cloudRender(ro, rd, bgCol, cloudHit, tr.d), 1.f);
#endif

    // === Case when main render did not hit a real object ===
    if (ri.isEnv && !cloudHit && !terrainHit &&!seaHit) {
        // NO HIT
        if (ri.isAL) {
           // If hit area lights
           setBrightness(ri.fragColor.rgb); fragColor = ri.fragColor; return;
        }
        fragColor = ri.fragColor; BrightColor = vec4(0.0, 0.0, 0.0, 1.0); return;
    } else if (cloudHit) {
        // If main render did not hit and we hit cloud
        setBrightness(cres.rgb); fragColor = cres; return;
    } else if (terrainHit) {
        // If main render did not hit and we hit terrain
        setBrightness(tr.fragColor.rgb); fragColor = tr.fragColor; return;
    } else if (seaHit) {
        // If main render did not hit and we hit sea
        setBrightness(sr.fragColor.rgb); fragColor = sr.fragColor; return;
    }
    // ========================================================

    phong = ri.fragColor;

    // =================== Refl && Refr =====================
    oi.intersectObj = info.intersectObj; oi.n = info.n; oi.p = info.p; oi.rd = info.rd;
    // === Secondary Rays ===
    RayMarchObject obj = objects[info.intersectObj];
    vec3 cRefl = obj.cReflective, cRefr = obj.cTransparent;
    if (obj.type == CUSTOM) {
        // Change accordingly
        if (ri.customId == 0) {
            cRefl = vec3(0.3);
        }
    }
    if (enableReflection && length(cRefl) != 0) {
        vec3 fil = vec3(1.f);
        // GLSL does not have recursion apparently :(
        // Here is my work around
        // - fil keeps track of the accumulated material reflectivity
        for (int i = 0; i < NUM_REFLECTION; i++) {
            // Reflect ray
            vec3 r = reflect(info.rd, info.n);
            vec3 shiftedRO = info.p + r * SURFACE_DIST * 3.f;
            // fil *= objects[info.intersectObj].cReflective;
            fil *= cRefl;
            // Render the reflected ray
            bool terrainHit = false, cloudHit = false, seaHit = false; vec4 cres;
            RenderInfo res, tr, sr;
            res = render(shiftedRO, r, info, OUTSIDE, far, bgCol);
            tr.d = res.d; sr.d = res.d;
#ifdef SEA
            sr = seaRender(shiftedRO, r, seaHit, res.d, bgCol);
            if (seaHit) { res.fragColor = sr.fragColor; sr.isEnv = true; }
#endif
#ifdef TERRAIN
            tr = terrainRender(shiftedRO, r, terrainHit, sr.d, bgCol);
            if (terrainHit) { res.fragColor = tr.fragColor; res.isEnv = true; }
#endif
#ifdef CLOUD
            cres = vec4(cloudRender(shiftedRO, r, bgCol, cloudHit, tr.d), 1.f);
            if (cloudHit) { res.fragColor = cres; res.isEnv = true; }
#endif
            // Always want to incorporate the first reflected ray's color val
            vec4 bounce = vec4(ks * fil * vec3(res.fragColor), 1.f);
            refl += bounce;
            if (res.isEnv) break;
        }
    }

    if (enableRefraction && length(cRefr) != 0) {
        // No recursion so hardcoded 2 refractions :(
        // also it does not account for the reflected light contributions
        // of the refracted rays (again, no recursion) so this is really wrong.
        // I was able to find some article about backward raytracing that cleverly
        // gets around this but, like many other things, for the time being this is
        // good enough.

        float ior = objects[oi.intersectObj].ior;
        vec3 ct = objects[oi.intersectObj].cTransparent;

        // Air -> Medium
        // - air ior is 1.
        vec3 rdIn = refract(oi.rd, oi.n, 1./ior);
        vec3 pEnter = oi.p - oi.n * SURFACE_DIST * 3.f;
        float dIn = raymarch(pEnter, rdIn, far, INSIDE).d;

        vec3 pExit = pEnter + rdIn * dIn;
        vec3 nExit = -getNormal(pExit);

        vec3 rdOut = refract(rdIn, nExit, ior);
        if (length(rdOut) == 0) {
            // Total Internal Reflection
            refr = vec4(0.);
        } else {
            vec3 shiftedRO = pExit - nExit * SURFACE_DIST*5.f;
            bool cloudHit = false, terrainHit = false, seaHit = false;
            vec4 cres; vec4 resC; RenderInfo res, tr, sr;
            res = render(shiftedRO, rdOut, info, OUTSIDE, far, bgCol);
            tr.d = res.d; sr.d = res.d;
#ifdef SEA
            sr = seaRender(shiftedRO, rdOut, seaHit, res.d, bgCol);
            if (seaHit) res.fragColor = sr.fragColor;
#endif
#ifdef TERRAIN
            tr = terrainRender(shiftedRO, rdOut, terrainHit, sr.d, bgCol);
            if (terrainHit) res.fragColor = tr.fragColor;
#endif
#ifdef CLOUD
            cres = vec4(cloudRender(shiftedRO, rdOut, bgCol, cloudHit, tr.d), 1.f);
            if (cloudHit) res.fragColor = cres;
#endif
            refr += vec4(kt * ct * vec3(res.fragColor), 1.f);
        }
    }


    vec4 col = phong + refl + refr;
    setBrightness(vec3(col));
    fragColor = col;
}
