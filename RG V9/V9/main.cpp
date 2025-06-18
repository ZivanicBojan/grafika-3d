#define _CRT_SECURE_NO_WARNINGS // Za Visual Studio, da isklju?i upozorenja za "nesigurne" funkcije

#include <iostream>
#include <fstream>   // Za ?itanje fajlova (shadera)
#include <sstream>   // Za rad sa string streamovima
#include <string>
#include <vector>

// Biblioteke za OpenGL
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// GLM biblioteke
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Uklju?ivanje stb_image.h biblioteke
// VAŽNO: Ova linija #define STB_IMAGE_IMPLEMENTATION mora biti samo u JEDNOM .cpp fajlu u projektu.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Osigurajte da je stb_image.h fajl u vašem projektu

// --- Globalne varijable ---

// Globalne varijable za kameru i rotaciju svetionika
float cameraAngleAroundLighthouse = 0.0f;
float cameraDistance = 5.0f;
float cameraHeight = 2.0f;
float cameraSpeed = 2.0f;
float zoomLevel = 45.0f;

bool lighthouseRotationPaused = false;
float lighthouseRotationSpeed = 50.0f;

// Globalne varijable za tekst (ime, prezime, indeks)
unsigned int textShaderProgram;
unsigned int fontTexture; // ID teksture za font
unsigned int textVAO, textVBO;

// Globalne varijable za dimenzije prozora (dodato, jer se koriste u renderText)
const unsigned int wWidth = 800;
const unsigned int wHeight = 600;

// Globalne varijable za 2D pravougaonik sa teksturom
unsigned int rectVAO, rectVBO;    // VAO i VBO za pravougaonik
unsigned int myImageTextureID;    // ID vaše u?itane slike/teksture
unsigned int textureRectShader;   // Shader program za renderovanje teksture na pravougaoniku

// --- Deklaracije funkcija ---

// Funkcije za shadere
unsigned int compileShader(GLenum type, const char* sourcePath);
unsigned int createShader(const char* vsSourcePath, const char* fsSourcePath); // ?ita iz fajlova
unsigned int createShaderFromSource(const char* vsSource, const char* fsSource); // Direktno iz stringova (za tekst)

// Funkcije za kreiranje geometrije
void createCube(unsigned int& VAO, unsigned int& VBO);
void createPlane(unsigned int& VAO, unsigned int& VBO);
void createPyramid(unsigned int& VAO, unsigned int& VBO);

// Callback funkcije
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

// Funkcija za u?itavanje teksture pomo?u stb_image
GLuint loadImageTexture(const char* filename);



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

    // Kreiraj shader program za 3D objekte
    unsigned int shaderProgram = createShader("basic.vert", "basic.frag");
    glUseProgram(shaderProgram);

    

    float rectVertices[] = {
        // Pozicija (x, y, z)    // UV koordinate (s, t)
        // Prvi trokut
        -0.95f,  0.95f, 0.0f,    0.0f, 1.0f,
        -0.95f,  0.75f, 0.0f,    0.0f, 0.0f, 
        -0.55f,  0.75f, 0.0f,    1.0f, 0.0f, 

        // Drugi trokut
        -0.95f,  0.95f, 0.0f,    0.0f, 1.0f, 
        -0.55f,  0.75f, 0.0f,    1.0f, 0.0f, 
        -0.55f,  0.95f, 0.0f,    1.0f, 1.0f  
    };

    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);

    // Konfiguracija atributa verteksa
    // Atribut 0: Pozicije (3 float-a)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Atribut 1: Teksturne koordinate (2 float-a)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // 2. U?itavanje vaše PNG teksture
    // PRILAGODITE OVU PUTANJU PREMA LOKACIJI VAŠE SLIKE!
    stbi_set_flip_vertically_on_load(true);
    myImageTextureID = loadImageTexture("name.png"); // Npr. ako je u res/textures folderu

    // 3. Kompajliranje shadera za teksturu iz fajlova
    // PRILAGODITE OVU PUTANJU PREMA LOKACIJI VAŠIH SHADER FAJLOVA!
    textureRectShader = createShader("name.vert", "name.frag");
    glUseProgram(textureRectShader);
    glUniform1i(glGetUniformLocation(textureRectShader, "textureSampler"), 0); // Postavite sampler uniform na texture unit 0
    glUseProgram(0); // Vratite se na podrazumijevani program


    // Kreiraj geometrije (3D objekti)
    unsigned int cubeVAO, cubeVBO_obj;
    createCube(cubeVAO, cubeVBO_obj);

    unsigned int planeVAO, planeVBO_obj;
    createPlane(planeVAO, planeVBO_obj);

    unsigned int pyramidVAO, pyramidVBO_obj;
    createPyramid(pyramidVAO, pyramidVBO_obj);

    // Uniform lokacije za 3D objekte
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

        // --- Renderovanje 3D objekata ---
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

        // --- Renderovanje 2D pravougaonika sa vašom teksturom ---
        glDisable(GL_DEPTH_TEST); // Onemogu?i testiranje dubine (2D elementi se crtaju preko svega)
        glEnable(GL_BLEND);       // Omogu?i blending za transparentnost (ako vaša PNG slika ima alpha kanal)
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Standardni blending mod

        glUseProgram(textureRectShader); // Aktivira shader program za teksturu pravougaonika

        glActiveTexture(GL_TEXTURE0); // Aktivira texture unit 0
        glBindTexture(GL_TEXTURE_2D, myImageTextureID); // Veži vašu u?itanu teksturu na active unit

        glBindVertexArray(rectVAO);      // Veži VAO našeg pravougaonika
        glDrawArrays(GL_TRIANGLES, 0, 6); // Crtaj 6 verteksa (2 trokuta koja ?ine pravougaonik)

        glBindVertexArray(0);           // Odveži VAO
        glBindTexture(GL_TEXTURE_2D, 0); // Odveži teksturu
        glUseProgram(0);                 // Odveži shader program

       
        // Ponovo omogu?i testiranje dubine i face culling za 3D objekte
        glDisable(GL_BLEND); // Onemogu?i blending
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- Cleanup ---
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

    // Cleanup za 2D pravougaonik sa teksturom
    glDeleteVertexArrays(1, &rectVAO);
    glDeleteBuffers(1, &rectVBO);
    glDeleteProgram(textureRectShader);
    glDeleteTextures(1, &myImageTextureID);

    glfwTerminate();
    return 0;
}

// --- Implementacije pomo?nih funkcija (isti kao u vašem kodu, uz manje ispravke za ?itanje fajlova) ---

// Callback funkcija za obradu unosa tastature
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

// Callback funkcija za skrolanje miša (zumiranje)
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
        std::cerr << "Greska pri otvaranju fajla: " << sourcePath << ". Da li postoji?" << std::endl;
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
unsigned int createShader(const char* vsSourcePath, const char* fsSourcePath)
{
    unsigned int program = glCreateProgram();

    unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vsSourcePath);
    unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSourcePath);

    if (vertexShader == 0 || fragmentShader == 0) { // Provjera greške pri kompilaciji
        glDeleteProgram(program);
        return 0;
    }

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

// Funkcija za u?itavanje teksture pomo?u stb_image
GLuint loadImageTexture(const char* filename) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Postavite wrapping i filtering opcije
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // Za mipmape i glatke prelaze
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);             // Za uve?anje

    int width, height, nrChannels;
    // Opciono: Ako vam je slika okrenuta naopako, odkomentirajte sljede?u liniju
    // stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = GL_RGB; // Default
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA; // PNG ?esto ima 4 kanala (RGBA)

        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D); // Generišite mipmape
    }
    else {
        std::cerr << "Failed to load texture: " << filename << std::endl;
    }
    stbi_image_free(data); // Oslobodite memoriju slike

    return textureID;
}

// Implementacije funkcija za kreiranje geometrije (ostaju iste)
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

