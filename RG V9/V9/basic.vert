#version 330 core
layout (location = 0) in vec3 aPos; // Ulazna pozicija vrha

uniform mat4 model;      // Matrica modela
uniform mat4 view;       // Matrica pogleda (kamere)
uniform mat4 projection; // Matrica projekcije

void main()
{
    // Kona?na pozicija vrha u prostoru klipa
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}