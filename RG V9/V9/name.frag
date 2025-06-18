#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D textureSampler; // Vaša tekstura

void main()
{
    FragColor = texture(textureSampler, TexCoord); // Uzorkuj teksturu koriste?i primljene UV koordinate
}