#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

//GLM biblioteke
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Globalne varijable za kameru i rotaciju
float cameraAngleAroundLighthouse = 0.0f; // Ugao rotacije kamere oko svetionika
float cameraDistance = 5.0f;             // Udaljenost kamere od svetionika
float cameraHeight = 2.0f;               // Visina kamere iznad centra svetionika
float cameraSpeed = 2.0f;                // Brzina rotacije/kretanja kamere (stepeni po tasteru ili jedinice udaljenosti)
float zoomLevel = 45.0f;                 // Po?etni FOV (Field of View) za zumiranje

bool lighthouseRotationPaused = false;   // Zastava za pauziranje rotacije svetionika
float lighthouseRotationSpeed = 50.0f;   // Brzina rotacije svetionika (stepeni po sekundi)

// Globalne varijable za tekst (ime, prezime, indeks)
unsigned int textShaderProgram;
unsigned int fontTexture; // ID teksture za font
unsigned int textVAO, textVBO;

// Globalne varijable za dimenzije prozora (dodato, jer se koriste u renderText)
const unsigned int wWidth = 800;
const unsigned int wHeight = 600;

// Funkcije za shadere
unsigned int compileShader(GLenum type, const char* sourcePath);
unsigned int createShader(const char* vsSource, const char* fsSource);
unsigned int createShaderFromSource(const char* vsSource, const char* fsSource); // Za tekst shadere

// Funkcije za kreiranje geometrije
void createCube(unsigned int& VAO, unsigned int& VBO);
void createPlane(unsigned int& VAO, unsigned int& VBO);
void createPyramid(unsigned int& VAO, unsigned int& VBO);

// Callback funkcija za obradu unosa tastature
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// Callback funkcija za skrolanje miša (zumiranje)
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// Funkcija za inicijalizaciju tekst renderera
void initTextRenderer(unsigned int& shaderProgram, unsigned int& textureID, unsigned int& VAO, unsigned int& VBO);
// Funkcija za renderiranje teksta
void renderText(const std::string& text, float x, float y, float scale, unsigned int shaderProgram, unsigned int textureID, unsigned int VAO);

int main(void)
{
    if (!glfwInit())
    {
        std::cout << "GLFW nije inicijalizovan!\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    window = glfwCreateWindow(wWidth, wHeight, "OpenGL Example", NULL, NULL);

    if (window == NULL)
    {
        std::cout << "Nije moguce kreirati prozor!\n";
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window);

    // Ograni?enje na 60 FPS (V-Sync)
    glfwSwapInterval(1);

    // Registracija callback funkcija
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);

    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW nije inicijalizovan!\n";
        return 3;
    }

    // Kreiraj shader program za objekte
    unsigned int shaderProgram = createShader("basic.vert", "basic.frag");
    glUseProgram(shaderProgram);

    // Inicijaliziraj text renderer (za ime, prezime, indeks)
    initTextRenderer(textShaderProgram, fontTexture, textVAO, textVBO);

    // Kreiraj geometrije
    unsigned int cubeVAO, cubeVBO_obj;
    createCube(cubeVAO, cubeVBO_obj);

    unsigned int planeVAO, planeVBO_obj;
    createPlane(planeVAO, planeVBO_obj);

    unsigned int pyramidVAO, pyramidVBO_obj;
    createPyramid(pyramidVAO, pyramidVBO_obj);

    // Uniform lokacije za objekte
    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "color");

    // Omogu?i testiranje dubine
    glEnable(GL_DEPTH_TEST);
    // Omogu?i odstranjivanje lica
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK); // Odstranjuj zadnja lica (ona okrenuta od kamere)
    glFrontFace(GL_CCW); // Prednja lica su ona koja su definirana u smjeru suprotnom od kazaljke na satu (Counter-Clockwise)

    // Vrijeme za rotaciju svjetionika i FPS kontrolu
    float lastFrame = 0.0f;

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glClearColor(0.2f, 0.25f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- Renderiranje 3D objekata ---
        glUseProgram(shaderProgram);

        // Ažuriraj projekciju za zumiranje
        glm::mat4 projection = glm::perspective(glm::radians(zoomLevel), (float)wWidth / wHeight, 0.1f, 100.0f);
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Pozicija vrha svetionika oko koje se kamera rotira
        glm::vec3 lighthouseTarget = glm::vec3(0.0f, 2.5f, 0.0f); // Pretpostavka da je vrh svjetionika na (0, 2.5, 0)

        // Ažuriraj poziciju kamere oko svetionika
        glm::vec3 cameraPos = glm::vec3(
            lighthouseTarget.x + cameraDistance * glm::cos(glm::radians(cameraAngleAroundLighthouse)),
            cameraHeight,
            lighthouseTarget.z + cameraDistance * glm::sin(glm::radians(cameraAngleAroundLighthouse))
        );
        glm::mat4 view = glm::lookAt(cameraPos, lighthouseTarget, glm::vec3(0.0f, 1.0f, 0.0f));
        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

        // Crtaj okean (velika plava ravan)
        glm::mat4 modelOcean = glm::scale(glm::mat4(1.0f), glm::vec3(50.0f, 1.0f, 50.0f));
        modelOcean = glm::translate(modelOcean, glm::vec3(0.0f, -0.5f, 0.0f)); // Pomjeri ispod ostrva
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelOcean));
        glUniform3f(colorLoc, 0.0f, 0.4f, 0.7f); // Plava boja za okean
        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO_obj);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Crtaj ostrvo (sme?a ravan)
        glm::mat4 modelIsland = glm::scale(glm::mat4(1.0f), glm::vec3(5.0f, 1.0f, 5.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelIsland));
        glUniform3f(colorLoc, 0.6f, 0.4f, 0.2f); // Sme?a boja za ostrvo
        glBindVertexArray(planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, planeVBO_obj);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Crtaj bazu svetionika (kocke)
        glm::mat4 modelLighthouseBase = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));
        modelLighthouseBase = glm::scale(modelLighthouseBase, glm::vec3(0.8f, 1.0f, 0.8f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelLighthouseBase));
        glUniform3f(colorLoc, 0.8f, 0.8f, 0.8f); // Siva boja za bazu
        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_obj);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Crtaj srednji dio svetionika (kocke)
        glm::mat4 modelLighthouseMid = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, 0.0f));
        modelLighthouseMid = glm::scale(modelLighthouseMid, glm::vec3(0.6f, 1.0f, 0.6f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelLighthouseMid));
        glUniform3f(colorLoc, 0.7f, 0.7f, 0.7f); // Tamnija siva boja
        glBindVertexArray(cubeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO_obj);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Crtaj gornji dio svetionika (rotiraju?a piramida)
        glm::mat4 modelLighthouseTop = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f));
        if (!lighthouseRotationPaused)
        {
            // Rotiraj gornji dio oko svoje y-ose
            modelLighthouseTop = glm::rotate(modelLighthouseTop, glm::radians((float)glfwGetTime() * lighthouseRotationSpeed), glm::vec3(0.0f, 1.0f, 0.0f));
        }
        modelLighthouseTop = glm::scale(modelLighthouseTop, glm::vec3(0.9f, 1.05f, 0.9f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelLighthouseTop));
        glUniform3f(colorLoc, 1.0f, 1.0f, 0.0f); // Žuta boja za vrh
        glBindVertexArray(pyramidVAO);
        glBindBuffer(GL_ARRAY_BUFFER, pyramidVBO_obj);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        // --- Renderiranje teksta ---
        // Onemogu?i testiranje dubine za tekst da bi uvijek bio na vrhu
        glDisable(GL_DEPTH_TEST);
        // Onemogu?i face culling za tekst
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND); // Omogu?i blending za transparentnost teksta
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        renderText("IME PREZIME", 10.0f, (float)wHeight - 30.0f, 0.5f, textShaderProgram, fontTexture, textVAO);
        renderText("Indeks: XXXX/YYYY", 10.0f, (float)wHeight - 60.0f, 0.5f, textShaderProgram, fontTexture, textVAO);

        // Ponovo omogu?i testiranje dubine i face culling za 3D objekte
        glDisable(GL_BLEND); // Onemogu?i blending
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO_obj);

    glDeleteVertexArrays(1, &planeVAO);
    glDeleteBuffers(1, &planeVBO_obj);

    glDeleteVertexArrays(1, &pyramidVAO);
    glDeleteBuffers(1, &pyramidVBO_obj);

    glDeleteProgram(shaderProgram);

    // Cleanup za tekst
    glDeleteVertexArrays(1, &textVAO);
    glDeleteBuffers(1, &textVBO);
    glDeleteProgram(textShaderProgram);
    glDeleteTextures(1, &fontTexture);

    glfwTerminate();
    return 0;
}

// Implementacija callback funkcija

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        switch (key)
        {
        case GLFW_KEY_W: // Smanji udaljenost kamere (približi se)
            cameraDistance -= cameraSpeed * 0.1f; // Manji korak
            if (cameraDistance < 1.0f) cameraDistance = 1.0f; // Minimalna udaljenost
            break;
        case GLFW_KEY_S: // Pove?aj udaljenost kamere (udalji se)
            cameraDistance += cameraSpeed * 0.1f; // Manji korak
            if (cameraDistance > 10.0f) cameraDistance = 10.0f; // Maksimalna udaljenost
            break;
        case GLFW_KEY_A: // Rotiraj kameru lijevo
            cameraAngleAroundLighthouse -= cameraSpeed;
            break;
        case GLFW_KEY_D: // Rotiraj kameru desno
            cameraAngleAroundLighthouse += cameraSpeed;
            break;
        case GLFW_KEY_SPACE:
            if (action == GLFW_PRESS) { // Provjeri da li je tipka pritisnuta, a ne držana
                lighthouseRotationPaused = !lighthouseRotationPaused;
            }
            break;
        case GLFW_KEY_ESCAPE: // Izlazak iz aplikacije
            glfwSetWindowShouldClose(window, true);
            break;
        }
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    zoomLevel -= (float)yoffset * 2.0f; // Podesi osjetljivost zumiranja
    if (zoomLevel < 1.0f) zoomLevel = 1.0f;     // Minimalni FOV (najve?e zumiranje)
    if (zoomLevel > 90.0f) zoomLevel = 90.0f;   // Maksimalni FOV (najmanje zumiranje)
}

// Funkcija za kompilaciju shadera iz fajla
unsigned int compileShader(GLenum type, const char* sourcePath)
{
    std::ifstream file(sourcePath);
    if (!file.is_open()) {
        std::cerr << "Greska pri otvaranju fajla: " << sourcePath << std::endl;
        return 0;
    }
    std::stringstream ss;
    ss << file.rdbuf();
    std::string sourceStr = ss.str();
    const char* source = sourceStr.c_str();

    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Greska kompajliranja shader-a (" << sourcePath << "):\n" << infoLog << std::endl;
    }
    return shader;
}

// Funkcija za kreiranje shader programa iz fajlova
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    unsigned int program = glCreateProgram();

    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Greska linkovanja shader programa:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// Funkcija za kreiranje shader programa direktno iz stringova (za tekst)
unsigned int createShaderFromSource(const char* vsSource, const char* fsSource)
{
    unsigned int program = glCreateProgram();

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vsSource, NULL);
    glCompileShader(vertexShader);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Greska kompajliranja vertex shadera za tekst:\n" << infoLog << std::endl;
    }


    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fsSource, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Greska kompajliranja fragment shadera za tekst:\n" << infoLog << std::endl;
    }


    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Greska linkovanja shader programa za tekst:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

// Geometrija
// (funkcije createCube, createPlane, createPyramid ostaju iste kao u vašem kodu)

void createCube(unsigned int& VAO, unsigned int& VBO)
{
    float vertices[] = {
        // pozicije (x,y,z)
        // Prednja strana
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,

        // Zadnja strana
        -0.5f, -0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,

        // Lijeva strana
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,

        // Desna strana
         0.5f,  0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,

         // Gornja strana
         -0.5f,  0.5f,  0.5f,
          0.5f,  0.5f,  0.5f,
          0.5f,  0.5f, -0.5f,
          0.5f,  0.5f, -0.5f,
         -0.5f,  0.5f, -0.5f,
         -0.5f,  0.5f,  0.5f,

         // Donja strana
         -0.5f, -0.5f,  0.5f,
         -0.5f, -0.5f, -0.5f,
          0.5f, -0.5f, -0.5f,
          0.5f, -0.5f,  0.5f,
         -0.5f, -0.5f,  0.5f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void createPlane(unsigned int& VAO, unsigned int& VBO)
{
    float vertices[] = {
        // pozicije
        -0.5f, 0.0f,  0.5f,
         0.5f, 0.0f,  0.5f,
         0.5f, 0.0f, -0.5f,

         0.5f, 0.0f, -0.5f,
        -0.5f, 0.0f, -0.5f,
        -0.5f, 0.0f,  0.5f,
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void createPyramid(unsigned int& VAO, unsigned int& VBO)
{
    float vertices[] = {
        // Pozicije (x, y, z)
        // Baza (kvadrat)
        // Redoslijed vrhova za ispravan face culling (CCW)
        -0.5f, 0.0f, -0.5f,
         0.5f, 0.0f, -0.5f,
         0.5f, 0.0f,  0.5f,

         0.5f, 0.0f,  0.5f,
        -0.5f, 0.0f,  0.5f,
        -0.5f, 0.0f, -0.5f,

        // Strane (?etiri trougla) - Obratite pažnju na redoslijed vrhova za CCW
        // Prednja strana
        -0.5f, 0.0f,  0.5f,
         0.5f, 0.0f,  0.5f,
         0.0f, 0.8f,  0.0f,

         // Desna strana
          0.5f, 0.0f,  0.5f,
          0.5f, 0.0f, -0.5f,
          0.0f, 0.8f,  0.0f,

          // Zadnja strana
           0.5f, 0.0f, -0.5f,
          -0.5f, 0.0f, -0.5f,
           0.0f, 0.8f,  0.0f,

           // Lijeva strana
           -0.5f, 0.0f, -0.5f,
           -0.5f, 0.0f,  0.5f,
            0.0f, 0.8f,  0.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}


// --- Funkcije za renderiranje teksta ---

// Jednostavan font: ASCII karakteri od 32 do 126 (razmak do ~)
// Svaki karakter je 8x8 piksela u teksturi
// Bitmape za jednostavan 8x8 ASCII font
// U stvarnoj aplikaciji koristili biste FreeType ili sli?nu biblioteku
const unsigned char font8x8[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Space (32)
    0x00, 0x00, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00, // ! (33)
    0x00, 0x07, 0x00, 0x07, 0x00, 0x00, 0x00, 0x00, // " (34)
    0x14, 0x7E, 0x14, 0x7E, 0x14, 0x00, 0x00, 0x00, // # (35)
    0x24, 0x2A, 0x7F, 0x2A, 0x12, 0x00, 0x00, 0x00, // $ (36)
    0x63, 0x13, 0x08, 0x64, 0x62, 0x00, 0x00, 0x00, // % (37)
    0x36, 0x49, 0x55, 0x22, 0x50, 0x00, 0x00, 0x00, // & (38)
    0x00, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, // ' (39)
    0x00, 0x1C, 0x22, 0x41, 0x00, 0x00, 0x00, 0x00, // ( (40)
    0x00, 0x41, 0x22, 0x1C, 0x00, 0x00, 0x00, 0x00, // ) (41)
    0x00, 0x08, 0x3E, 0x08, 0x08, 0x00, 0x00, 0x00, // * (42)
    0x08, 0x08, 0x3E, 0x08, 0x08, 0x00, 0x00, 0x00, // + (43)
    0x00, 0x50, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, // , (44)
    0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00, 0x00, // - (45)
    0x00, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, // . (46)
    0x20, 0x10, 0x08, 0x04, 0x02, 0x00, 0x00, 0x00, // / (47)
    0x3E, 0x51, 0x49, 0x45, 0x3E, 0x00, 0x00, 0x00, // 0 (48)
    0x00, 0x42, 0x7F, 0x40, 0x00, 0x00, 0x00, 0x00, // 1 (49)
    0x42, 0x61, 0x51, 0x49, 0x46, 0x00, 0x00, 0x00, // 2 (50)
    0x21, 0x41, 0x45, 0x4B, 0x31, 0x00, 0x00, 0x00, // 3 (51)
    0x18, 0x14, 0x12, 0x7F, 0x10, 0x00, 0x00, 0x00, // 4 (52)
    0x27, 0x45, 0x45, 0x45, 0x39, 0x00, 0x00, 0x00, // 5 (53)
    0x3C, 0x4A, 0x49, 0x49, 0x30, 0x00, 0x00, 0x00, // 6 (54)
    0x01, 0x71, 0x09, 0x05, 0x03, 0x00, 0x00, 0x00, // 7 (55)
    0x36, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00, 0x00, // 8 (56)
    0x06, 0x49, 0x49, 0x29, 0x1E, 0x00, 0x00, 0x00, // 9 (57)
    0x00, 0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, // : (58)
    0x00, 0x56, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, // ; (59)
    0x08, 0x14, 0x22, 0x41, 0x00, 0x00, 0x00, 0x00, // < (60)
    0x14, 0x14, 0x14, 0x14, 0x14, 0x00, 0x00, 0x00, // = (61)
    0x41, 0x22, 0x14, 0x08, 0x00, 0x00, 0x00, 0x00, // > (62)
    0x02, 0x01, 0x51, 0x09, 0x06, 0x00, 0x00, 0x00, // ? (63)
    0x3E, 0x41, 0x5D, 0x59, 0x4E, 0x00, 0x00, 0x00, // @ (64)
    0x7C, 0x12, 0x11, 0x12, 0x7C, 0x00, 0x00, 0x00, // A (65)
    0x7F, 0x49, 0x49, 0x49, 0x36, 0x00, 0x00, 0x00, // B (66)
    0x3E, 0x41, 0x41, 0x41, 0x22, 0x00, 0x00, 0x00, // C (67)
    0x7F, 0x41, 0x41, 0x41, 0x3E, 0x00, 0x00, 0x00, // D (68)
    0x7F, 0x49, 0x49, 0x49, 0x41, 0x00, 0x00, 0x00, // E (69)
    0x7F, 0x09, 0x09, 0x09, 0x01, 0x00, 0x00, 0x00, // F (70)
    0x3E, 0x41, 0x49, 0x49, 0x7A, 0x00, 0x00, 0x00, // G (71)
    0x7F, 0x08, 0x08, 0x08, 0x7F, 0x00, 0x00, 0x00, // H (72)
    0x00, 0x41, 0x7F, 0x41, 0x00, 0x00, 0x00, 0x00, // I (73)
    0x20, 0x40, 0x41, 0x3F, 0x01, 0x00, 0x00, 0x00, // J (74)
    0x7F, 0x08, 0x14, 0x22, 0x41, 0x00, 0x00, 0x00, // K (75)
    0x7F, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, // L (76)
    0x7F, 0x02, 0x0C, 0x02, 0x7F, 0x00, 0x00, 0x00, // M (77)
    0x7F, 0x04, 0x08, 0x10, 0x7F, 0x00, 0x00, 0x00, // N (78)
    0x3E, 0x41, 0x41, 0x41, 0x3E, 0x00, 0x00, 0x00, // O (79)
    0x7F, 0x09, 0x09, 0x09, 0x06, 0x00, 0x00, 0x00, // P (80)
    0x3E, 0x41, 0x51, 0x21, 0x5E, 0x00, 0x00, 0x00, // Q (81)
    0x7F, 0x09, 0x19, 0x29, 0x46, 0x00, 0x00, 0x00, // R (82)
    0x46, 0x49, 0x49, 0x49, 0x31, 0x00, 0x00, 0x00, // S (83)
    0x01, 0x01, 0x7F, 0x01, 0x01, 0x00, 0x00, 0x00, // T (84)
    0x3F, 0x40, 0x40, 0x40, 0x3F, 0x00, 0x00, 0x00, // U (85)
    0x1F, 0x20, 0x40, 0x20, 0x1F, 0x00, 0x00, 0x00, // V (86)
    0x3F, 0x40, 0x38, 0x40, 0x3F, 0x00, 0x00, 0x00, // W (87)
    0x63, 0x14, 0x08, 0x14, 0x63, 0x00, 0x00, 0x00, // X (88)
    0x07, 0x08, 0x70, 0x08, 0x07, 0x00, 0x00, 0x00, // Y (89)
    0x61, 0x51, 0x49, 0x45, 0x43, 0x00, 0x00, 0x00, // Z (90)
    0x00, 0x7F, 0x41, 0x41, 0x00, 0x00, 0x00, 0x00, // [ (91)
    0x02, 0x04, 0x08, 0x10, 0x20, 0x00, 0x00, 0x00, // Backslash (92)
    0x00, 0x41, 0x41, 0x7F, 0x00, 0x00, 0x00, 0x00, // ] (93)
    0x04, 0x02, 0x01, 0x02, 0x04, 0x00, 0x00, 0x00, // ^ (94)
    0x40, 0x40, 0x40, 0x40, 0x40, 0x00, 0x00, 0x00, // _ (95)
    0x01, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, // ` (96)
    0x20, 0x54, 0x54, 0x54, 0x78, 0x00, 0x00, 0x00, // a (97)
    0x7F, 0x48, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00, // b (98)
    0x38, 0x44, 0x44, 0x44, 0x20, 0x00, 0x00, 0x00, // c (99)
    0x38, 0x44, 0x44, 0x48, 0x7F, 0x00, 0x00, 0x00, // d (100)
    0x38, 0x54, 0x54, 0x54, 0x18, 0x00, 0x00, 0x00, // e (101)
    0x08, 0x7E, 0x09, 0x01, 0x02, 0x00, 0x00, 0x00, // f (102)
    0x08, 0x14, 0x14, 0x14, 0x7C, 0x00, 0x00, 0x00, // g (103)
    0x7F, 0x08, 0x04, 0x04, 0x78, 0x00, 0x00, 0x00, // h (104)
    0x00, 0x44, 0x7D, 0x40, 0x00, 0x00, 0x00, 0x00, // i (105)
    0x20, 0x40, 0x44, 0x3D, 0x00, 0x00, 0x00, 0x00, // j (106)
    0x7F, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x00, // k (107)
    0x00, 0x41, 0x7F, 0x40, 0x00, 0x00, 0x00, 0x00, // l (108)
    0x7C, 0x04, 0x18, 0x04, 0x78, 0x00, 0x00, 0x00, // m (109)
    0x7C, 0x08, 0x04, 0x04, 0x78, 0x00, 0x00, 0x00, // n (110)
    0x38, 0x44, 0x44, 0x44, 0x38, 0x00, 0x00, 0x00, // o (111)
    0x7C, 0x14, 0x14, 0x14, 0x08, 0x00, 0x00, 0x00, // p (112)
    0x08, 0x14, 0x14, 0x18, 0x7C, 0x00, 0x00, 0x00, // q (113)
    0x7C, 0x08, 0x04, 0x04, 0x08, 0x00, 0x00, 0x00, // r (114)
    0x48, 0x54, 0x54, 0x54, 0x20, 0x00, 0x00, 0x00, // s (115)
    0x04, 0x3F, 0x44, 0x40, 0x20, 0x00, 0x00, 0x00, // t (116)
    0x3C, 0x40, 0x40, 0x20, 0x7C, 0x00, 0x00, 0x00, // u (117)
    0x1C, 0x20, 0x40, 0x20, 0x1C, 0x00, 0x00, 0x00, // v (118)
    0x3C, 0x40, 0x30, 0x40, 0x3C, 0x00, 0x00, 0x00, // w (119)
    0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, // x (120)
    0x0C, 0x50, 0x50, 0x50, 0x3C, 0x00, 0x00, 0x00, // y (121)
    0x44, 0x64, 0x54, 0x4C, 0x44, 0x00, 0x00, 0x00, // z (122)
    0x00, 0x08, 0x36, 0x41, 0x00, 0x00, 0x00, 0x00, // { (123)
    0x00, 0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, // | (124)
    0x00, 0x41, 0x36, 0x08, 0x00, 0x00, 0x00, 0x00, // } (125)
    0x08, 0x08, 0x2A, 0x1C, 0x08, 0x00, 0x00, 0x00, // ~ (126)
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Del (127)
};

// Vertex shader za renderiranje teksta (2D rendering)
const char* textVertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec4 vertex; // x, y, tex_x, tex_y

uniform mat4 projection;

out vec2 TexCoords;

void main()
{
    gl_Position = projection * vec4(vertex.xy, 0.0, 1.0);
    TexCoords = vertex.zw;
}
)";

// Fragment shader za renderiranje teksta (uzima teksturu)
const char* textFragmentShaderSource = R"(
#version 330 core
in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D text;
uniform vec3 textColor;

void main()
{
    vec4 sampled = vec4(1.0, 1.0, 1.0, texture(text, TexCoords).r); // Koristi crveni kanal teksture kao alpha
    FragColor = vec4(textColor, 1.0) * sampled;
}
)";

void initTextRenderer(unsigned int& shaderProgram, unsigned int& textureID, unsigned int& VAO, unsigned int& VBO_ref) {
    shaderProgram = createShaderFromSource(textVertexShaderSource, textFragmentShaderSource);
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "text"), 0); // Postavi uniform sampler2D na texturu unit 0
    glUniform3f(glGetUniformLocation(shaderProgram, "textColor"), 1.0f, 1.0f, 1.0f); // Bijela boja teksta (može se mijenjati)

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO_ref);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_ref);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Kreiraj teksturu za font
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Potrebno jer su pikseli 1 bajt
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, 768, 8, 0, GL_RED, GL_UNSIGNED_BYTE, font8x8);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // Vrati na default
    glBindTexture(GL_TEXTURE_2D, 0);
}

void renderText(const std::string& text, float x, float y, float scale, unsigned int shaderProgram, unsigned int textureID, unsigned int VAO) {
    glUseProgram(shaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glBindVertexArray(VAO);

    // Ovdje treba da postavimo orto projekciju za 2D tekst
    glm::mat4 projection = glm::ortho(0.0f, (float)wWidth, 0.0f, (float)wHeight); // Koristi globalne wWidth i wHeight
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    float charWidth = 8.0f * scale;
    float charHeight = 8.0f * scale;

    for (char c : text) {
        if (c < 32 || c > 127) continue; // Presko?i nepostoje?e karaktere

        float xpos = x;
        float ypos = y - charHeight; // Tekst ide od gornjeg lijevog ugla

        float tex_x = (float)(c - 32) * 8.0f / 768.0f; // Po?etna X koordinata u teksturi
        float tex_y = 0.0f; // Y koordinata je uvijek 0
        float tex_width = 8.0f / 768.0f; // Širina karaktera u teksturi
        float tex_height = 8.0f / 8.0f; // Visina karaktera u teksturi (uvijek 1.0)

        float vertices[6][4] = {
            {xpos,             ypos + charHeight, tex_x,           tex_y + tex_height},
            {xpos,             ypos,              tex_x,           tex_y},
            {xpos + charWidth, ypos,              tex_x + tex_width, tex_y},

            {xpos,             ypos + charHeight, tex_x,           tex_y + tex_height},
            {xpos + charWidth, ypos,              tex_x + tex_width, tex_y},
            {xpos + charWidth, ypos + charHeight, tex_x + tex_width, tex_y + tex_height}
        };

        glBindBuffer(GL_ARRAY_BUFFER, textVBO); // Koristi globalni textVBO
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Ažuriraj VBO podatke
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += charWidth; // Pomjeri X za sljede?i karakter
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}