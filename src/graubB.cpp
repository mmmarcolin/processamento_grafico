#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;
const int TILE_WIDTH = 128;
const int TILE_HEIGHT = 64;
int selectedX = 1, selectedY = 1;
int playerX = 1, playerY = 1;

int map[3][3] = {
    {1, 2, 1},
    {1, 2, 1},
    {1, 2, 2}
};

GLuint shaderProgram;
GLuint tilesetTexture;
GLuint vao, vbo;
GLuint playerTexture;

GLuint createShaderProgram() {
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;
        uniform mat4 model;
        uniform mat4 projection;
        out vec2 TexCoord;
        void main() {
            gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
            TexCoord = aTexCoord;
        }
    )";
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec2 TexCoord;
        uniform sampler2D tileset;
        void main() { FragColor = texture(tileset, TexCoord); }
    )";
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(vertexShader);
    glCompileShader(fragmentShader);
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    return program;
}

void loadTileset(const std::string& path) {
    glGenTextures(1, &tilesetTexture);
    glBindTexture(GL_TEXTURE_2D, tilesetTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);    
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    } else {
        std::cerr << "Erro ao carregar textura\n";
    }
}

void loadPlayerTexture(const std::string& path) {
    glGenTextures(1, &playerTexture);
    glBindTexture(GL_TEXTURE_2D, playerTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 4);
    if (data) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 4;
                if (data[idx + 3] < 128) {
                    data[idx + 0] = 0;
                    data[idx + 1] = 0;
                    data[idx + 2] = 0;
                    data[idx + 3] = 0;
                } else {
                    float alpha = data[idx + 3] / 255.0f;
                    data[idx + 0] = (unsigned char)(data[idx + 0] * alpha);
                    data[idx + 1] = (unsigned char)(data[idx + 1] * alpha);
                    data[idx + 2] = (unsigned char)(data[idx + 2] * alpha);
                }
            }
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    } else {
        std::cerr << "Erro ao carregar textura do personagem\n";
    }
}

void initBuffers() {
    float vertices[] = {
        0.0f, 0.0f,   0.0f, 1.0f,
        0.0f, 1.0f,   0.0f, 0.0f,
        1.0f, 1.0f,   1.0f, 0.0f,
        1.0f, 0.0f,   1.0f, 1.0f,
    };
    unsigned int indices[] = {0, 1, 2, 0, 2, 3};
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void drawTile(int tileIndex, int i, int j, glm::mat4 projection) {
    float tilesPerRow = 7.0f;
    float tileU = (float)tileIndex / tilesPerRow;
    float tileV = 0.0f;
    float tileUW = 1.0f / tilesPerRow;
    float tileVH = 1.0f; 
    glm::mat4 model = glm::mat4(1.0f);
    float screenX = (j - i) * (TILE_WIDTH / 2.0f);
    float screenY = (i + j) * (TILE_HEIGHT / 2.0f);
    model = glm::translate(model, glm::vec3(screenX + SCREEN_WIDTH / 2 - TILE_WIDTH / 2, screenY + SCREEN_HEIGHT / 4, 0.0f));
    model = glm::scale(model, glm::vec3(TILE_WIDTH, TILE_HEIGHT, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    float centerU = tileU + tileUW / 2.0f;
    float centerV = tileV + tileVH / 2.0f;
    float vertices[] = {
        0.5f, 0.5f, centerU, centerV,
        0.5f, 0.0f, centerU, tileV,
        1.0f, 0.5f, tileU + tileUW, centerV,
        0.5f, 1.0f, centerU, tileV + tileVH,
        0.0f, 0.5f, tileU, centerV,
        0.5f, 0.0f, centerU, tileV
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
}

void processInput(GLFWwindow* window) {
    static double lastMove = 0;
    double now = glfwGetTime();
    if (now - lastMove < 0.20) return; 
    int dx = 0, dy = 0;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) dy--;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) dy++;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) dx--;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) dx++;
    int nx = playerX + dx;
    int ny = playerY + dy;
    if (nx >= 0 && nx < 3 && ny >= 0 && ny < 3 && (dx != 0 || dy != 0)) {
        playerX = nx;
        playerY = ny;
        lastMove = now;
    }
}

void drawPlayer(int i, int j, glm::mat4 projection) {
    glm::mat4 model = glm::mat4(1.0f);
    float screenX = (j - i) * (TILE_WIDTH / 2.0f);
    float screenY = (i + j) * (TILE_HEIGHT / 2.0f);
    float px = screenX + SCREEN_WIDTH / 2 - TILE_WIDTH / 2;
    float py = screenY + SCREEN_HEIGHT / 4;
    float spriteW = TILE_WIDTH * 0.75f; 
    float spriteH = TILE_HEIGHT * 1.5f; 
    model = glm::translate(model, glm::vec3(px + (TILE_WIDTH - spriteW) / 2, py + (TILE_HEIGHT - spriteH) - TILE_HEIGHT / 4, 0.0f));
    model = glm::scale(model, glm::vec3(spriteW, spriteH, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    float vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, playerTexture);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Tilemap Isometrico", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    shaderProgram = createShaderProgram();
    glUseProgram(shaderProgram);
    loadTileset("../assets/tilesets/tilesetIso.png");
    loadPlayerTexture("../assets/sprites/Vampirinho.png");
    initBuffers();
    glm::mat4 projection = glm::ortho(0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f);
    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, tilesetTexture);
        for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) 
            drawTile(map[i][j], i, j, projection);
        drawPlayer(playerY, playerX, projection);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}