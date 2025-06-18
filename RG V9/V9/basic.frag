#version 330 core
out vec4 FragColor; // Izlazna boja fragmenta

uniform vec3 color; // Uniformna boja koja se postavlja iz CPU koda

void main()
{
    // Postavi boju fragmenta na uniformnu boju
    FragColor = vec4(color, 1.0f); // Posljednja komponenta je alpha (neprozirnost)
}