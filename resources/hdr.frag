#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D hdrBuffer;
uniform sampler2D bloomBlur;

uniform bool hdr;
uniform bool bloom;
uniform float exposure;

void main()
{
    // Either performs
    // 1. Gamma Correction
    // 2. HDR tone mapping
    // 3. Bloom
    const float gamma = 2.2;
    vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
    if (!hdr && !bloom) {
        // 1
        vec3 result = pow(hdrColor, vec3(1.0 / gamma));
        FragColor = vec4(result, 1.0);
        return;
    }
    vec3 bloomColor = texture(bloomBlur, TexCoords).rgb;
    if (bloom) {
        // 3.
        hdrColor += bloomColor;
    }
    // 2. / 3.
    vec3 result = vec3(1.0) - exp(-hdrColor * exposure);
    // result = pow(result, vec3(1.0 / gamma));
    FragColor = vec4(result, 1.0);
}
