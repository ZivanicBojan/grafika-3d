#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D textureSampler; // Va�a tekstura

void main()
{
    FragColor = texture(textureSampler, TexCoord); // Uzorkuj teksturu koriste?i primljene UV koordinate
}