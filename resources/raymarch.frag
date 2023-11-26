#version 330 core
// ============ CONST ==============
// - max raymarching steps
const int MAX_STEPS = 256;
// - threshold for intersection
const float SURFACE_DIST = 0.001;
// - small offset for the origin of shadow rays
const float SHADOWRAY_OFFSET = 0.007;
// - small eps for computing uv mapping
const float TEXTURE_EPS = 0.005;
// - PI
const float PI = 3.14159265;
// - reflection depth
const int NUM_REFLECTION = 3;

// PRIM TYPES
const int CUBE = 0;
const int CONE = 1;
const int CYLINDER = 2;
const int SPHERE = 3;
const int OCTAHEDRON = 4;
const int TORUS = 5;
const int CAPSULE = 6;
const int DEATHSTAR = 7;

// LIGHT TYPES
const int POINT = 0;
const int DIRECTIONAL = 1;
const int SPOT = 2;

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
};

struct SceneMin
{
    // Struct to store results of sdScene

    // Object with closest d
    int minObjIdx;
    // Largest step we can make
    float minD;
};

struct RayMarchRes
{
    // Struct to hold the result for a single Raymarch

    // Id of the intersected object
    int intersectObj;
    // Distance travelled along raydirection
    float d;
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

    int type;
    vec3 lightPos;
    vec3 lightDir;
    vec3 lightColor;
    vec3 lightFunc;
    float lightAngle;
    float lightPenumbra;
};

struct RenderInfo
{
    // Struct for holding render results.
    // We need this to differentiate the color values between true hit and environment
    vec3 fragColor;
    bool isEnv;
};

// =============== In ==============
in vec4 nearClip;
in vec4 farClip;

// =============== Out =============
out vec4 fragColor;

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
uniform float kt;
// - Scene Lights
uniform LightSource lights[10];
uniform int numLights;

// Objects
uniform RayMarchObject objects[50];
uniform int numObjects;

// Textures
uniform sampler2D objTextures[10];
uniform samplerCube skybox;

// Timer
uniform float iTime;

// Options
uniform bool enableGammaCorrection;
uniform bool enableSoftShadow;
uniform bool enableReflection;
uniform bool enableRefraction;
uniform bool enableAmbientOcculusion;
uniform bool enableSkyBox;

// ================== Utility =======================
// Transforms p along axis by angle
vec3 rotateAxis(vec3 p, vec3 axis, float angle) {
    return mix(dot(axis, p)*axis, p, cos(angle)) + cross(axis,p)*sin(angle);
}

// Applies transformation
vec3 Transform(in vec3 p)
{
//  p = rotateAxis(p, vec3(1, 0, 0), iTime);
  p = rotateAxis(p, vec3(0, 1, 0), iTime);
//  p = rotateAxis(p, vec3(0, 0, 1), iTime);
  return p;
}

// ============ Signed Distance Fields ==============
// Define SDF for different shapes here
// - Based on https://iquilezles.org/articles/distfunctions/

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
// @param h Half height of the cone
// @param r Radius of the base
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

float sdTorus(vec3 p, vec2 t)
{
    vec2 q = vec2(length(p.xz)-t.x,p.y);
    return length(q)-t.y;
}

float sdCapsule(vec3 p, float h, float r)
{
  p.y -= clamp(p.y, 0.0, h);
  return length(p) - r;
}

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
float sdMatch(vec3 p, int type)
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
        return sdOctahedron(Transform(p), 0.5);
    } else if (type == TORUS) {
        return sdTorus(p, vec2(0.5, 0.5/4));
    } else if (type == CAPSULE) {
        return sdCapsule(p, 0.5, 0.1);
    } else if (type == DEATHSTAR) {
        return sdDeathStar(p, 0.5, 0.35, 0.5);
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
    for (int i = 0; i < numObjects; i++) {
        // Get current obj
        RayMarchObject obj = objects[i];
        // Conv to Object space
        // - (Note) for global transformation
        //po = vec3(obj.invModelMatrix * vec4(Transform(p), 1.f));
        po = vec3(obj.invModelMatrix * vec4(p, 1.f));

        // Get the distance to the object
        currD = sdMatch(po, obj.type) * obj.scaleFactor;
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
  } else {
      // NO HIT
      res.intersectObj = -1;
  }
  return res;
}

// =============================================================

// ================== Phong Total Illumination =================
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
    for( int i=0; i<MAX_STEPS && rayDepth<maxt; i++ )
    {
        float h = sdScene(ro + rd*rayDepth).minD;
        if( h< SURFACE_DIST ) {
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

        // Shadow
        RayMarchRes res = softshadow(p + N * SURFACE_DIST * 3., L, 0, far, 8);
        if (res.intersectObj == 1) {
            // Shadow Ray intersected an object
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
        vec3 dirToCamera = normalize(-rd);
        float RdotV = clamp(dot(R, dirToCamera), 0.f, 1.f);
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
// Given ray origin and ray direction, performs a raymarching
// @param ro Ray origin
// @param rd Ray direction
// @param i IntersectionInfo we are populating
// @param side Determines if we are inside or outside of an object
RenderInfo render(in vec3 ro, in vec3 rd, out IntersectionInfo i, in float side)
{
    RenderInfo ri;
    // Raymarching
    RayMarchRes res = raymarch(ro, rd, far, side);
    if (res.intersectObj == -1) {
        // If no hit but sky box is used, sample
        if (enableSkyBox) {
            ri.fragColor = vec3(texture(skybox, rd).rgb);
            ri.isEnv = true;
            return ri;
        } else {
            // Simple black screen
            ri.fragColor = vec3(0.f);
            ri.isEnv = true;
            return ri;
        }
    }

    // HIT
    // - hit point
    vec3 p = ro + rd * res.d;
    // - normalized hit normal
    vec3 pn = getNormal(p);
    // - get color
    vec3 col = getPhong(pn, res.intersectObj, p, rd);

    i.p = ro + rd * res.d;
    i.n = pn;
    i.rd = rd;
    i.intersectObj = res.intersectObj;

    ri.fragColor = col;
    ri.isEnv = false;

    return ri;
}


void main() {
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

    if (enableGammaCorrection) {
        col = pow(col, vec3(1.0/2.2));
    }
    fragColor = vec4(col, 1.f);
}
