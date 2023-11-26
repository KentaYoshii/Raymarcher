#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexcoord;

uniform bool useModel;
uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projMatrix;

void main()
{
    if (useModel) {
        gl_Position = projMatrix * viewMatrix * modelMatrix * vec4(aPosition, 1.0f);
    } else {
        gl_Position = projMatrix * viewMatrix * vec4(aPosition, 1.0f);
    }
}
