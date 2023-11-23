#version 330 core

const int MAX_STEPS = 50;
const float MAX_DIST = 100.0;
const float SURFACE_DIST = 0.001;

// In
in vec4 nearClip;
in vec4 farClip;

// Out
out vec4 fragColor;

// Uniforms
uniform vec2 screenDimensions;
//uniform float farPlane;
uniform vec4 eyePosition;

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


// ==================================================

// Union of all SDFs
float sdScene(vec3 p){
    // float d = sdSphere(p, 0.5f);
    // float d = sdBox(p, vec3(0.5, 0.5, 0.5));
    // float d = sdCone(p, 0.5, 0.5);
    float d = sdCylinder(p, 0.5, 0.5);
    return d;
}

// Raymarch algorithm
float raymarch(vec3 ro, vec3 rd) {
  // Start from eye pos
  float dO = 0.0;
  // Start the march
  for(int i = 0; i < MAX_STEPS; i++) {
    vec3 p = ro + rd * dO;
    float dS = sdScene(p);

    dO += dS;

    if(dO > MAX_DIST || dS < SURFACE_DIST) {
        break;
    }
  }
  return dO;
}

void main() {
    // Perspective divide
    vec3 origin = nearClip.xyz / nearClip.w;
    vec3 far = farClip.xyz / farClip.w;
    vec3 dir = far - origin;
    dir = normalize(dir);

    // Raymarching
    float d = raymarch(origin, dir);
    vec3 p = origin + dir * d;


    // Color
    vec3 color = vec3(0.f);
    if (d < MAX_DIST) {
        color = vec3(1.f);
    }

    fragColor = vec4(color, 1.f);
}
