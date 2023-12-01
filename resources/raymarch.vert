#version 330 core
layout (location = 0) in vec2 pos;

// Uniform
uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 invProjViewMatrix;

out vec4 nearClip;
out vec4 farClip;
out vec2 twoDFragCoord;

void main() {
    // Simple UV
    gl_Position = vec4(pos, 0, 1.f);

    // For 2d
    twoDFragCoord = pos;

    // (REFERENCE)
    // https://community.khronos.org/t/ray-origin-through-view-and-projection-matrices/72579/4
    // In NDC, all ray intersects at (x, y, -1) and (x, y, 1)    
    nearClip = invProjViewMatrix * (vec4(gl_Position.xy, -1, 1.0));
    farClip = invProjViewMatrix * (vec4(gl_Position.xy, +1, 1.0));
}
