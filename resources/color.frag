#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D debugTexture;

void main()
{
    // Sample the debug texture at the current texture coordinate
    vec4 debugColor = texture(debugTexture, TexCoords);

    // Output the sampled color for visualization
    FragColor = debugColor;
}
