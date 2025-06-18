#version 330 core
layout (location = 0) in vec3 aPos;     // Pozicija vertexa (x, y, z)
layout (location = 1) in vec2 aTexCoord; // UV koordinate

out vec2 TexCoord;                      // Proslije?ujemo UV koordinate fragment shaderu

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0); // Pravougaonik je ve? u NDC, tako da ne trebaju matrice
    TexCoord = aTexCoord;
}