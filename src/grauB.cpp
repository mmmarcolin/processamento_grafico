// --- INCLUDE AND LIBRARY DEFINITIONS ---
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
#include <fstream>
#include <sstream>
#include <algorithm>
#include <chrono>

// --- TILE STRUCTURE ---
struct TileInfo { int tileIndex; bool hasCoin; };

// --- SCREEN AND TILE CONSTANTS ---
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;
const int TILE_WIDTH = 64;
const int TILE_HEIGHT = 32;

// --- GAME STATE VARIABLES ---
int playerX = 1, playerY = 1;
int mapRows = 0, mapCols = 0;
int coinCount = 0, totalCoins = 0;
double startTime = 0;
int coinFrame = 0, playerIdleFrame = 0;
double coinAnimTimer = 0, playerIdleTimer = 0;
const double coinAnimSpeed = 0.06, playerIdleSpeed = 0.18;

// --- RESOURCE AND OPENGL VARIABLES ---
std::string tilesetFile;
std::vector<std::vector<TileInfo>> mapData;
GLuint shaderProgram, tilesetTexture, vao, vbo;
GLuint playerTexture, playerIdleTexture;
GLuint coinTextures[10];
enum GameState { RUNNING, WON, GAMEOVER };
GameState gameState = RUNNING;
glm::mat4 projection;

// --- SHADER PROGRAM CREATION ---
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
        uniform vec4 colorMod;
        void main() { FragColor = texture(tileset, TexCoord) * colorMod; }
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

// --- MAP LOADING FUNCTION ---
void loadMapFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Erro ao abrir " << path << std::endl;
        return;
    }
    std::string line;
    std::getline(file, tilesetFile);
    std::getline(file, line);
    std::istringstream sizeStream(line);
    sizeStream >> mapRows >> mapCols;
    mapData.clear();
    for (int i = 0; i < mapRows; ++i) {
        std::getline(file, line);
        std::istringstream rowStream(line);
        std::vector<TileInfo> row;
        for (int j = 0; j < mapCols; ++j) {
            std::string token;
            rowStream >> token;
            TileInfo info;
            if (!token.empty() && token[0] == '/') continue; 
            if (!token.empty() && token.back() == 'c') {
                info.hasCoin = true;
                token.pop_back();
            } else {
                info.hasCoin = false;
            }
            if (!token.empty()) info.tileIndex = std::stoi(token);
            else                info.tileIndex = 0;
            row.push_back(info);
        }
        mapData.push_back(row);
    }
}

// --- TILESET TEXTURE LOADING ---
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

// --- PLAYER TEXTURE LOADING ---
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

// --- COIN ANIMATION TEXTURES LOADING ---
void loadCoinTextures() {
    char buf[128];
    for (int i = 0; i < 10; ++i) {
        sprintf(buf, "../assets/sprites/Gold_%d.png", i+21);
        glGenTextures(1, &coinTextures[i]);
        glBindTexture(GL_TEXTURE_2D, coinTextures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        int width, height, nrChannels;
        unsigned char* data = stbi_load(buf, &width, &height, &nrChannels, 4);
        if (data) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        } else {
            printf("Erro ao carregar %s\n", buf);
        }
    }
}

// --- PLAYER IDLE ANIMATION TEXTURE LOADING ---
void loadPlayerIdleTexture() {
    glGenTextures(1, &playerIdleTexture);
    glBindTexture(GL_TEXTURE_2D, playerIdleTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    int width, height, nrChannels;
    unsigned char* data = stbi_load("../assets/sprites/Vampires1_Idle_full.png", &width, &height, &nrChannels, 4);
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    } else {
        printf("Erro ao carregar Vampires1_Idle_full.png\n");
    }
}

// --- OPENGL BUFFER INITIALIZATION ---
void initBuffers() {
    float vertices[] = {
        0.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
        1.0f, 0.0f, 1.0f, 1.0f,
    };
    unsigned int indices[] = { 0, 1, 2, 0, 2, 3 };
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

// --- TILE DRAWING FUNCTION ---
void drawTile(int tileIndex, int i, int j, glm::mat4 projection, bool darken = false) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tilesetTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "tileset"), 0);
    if (darken) glUniform4f(glGetUniformLocation(shaderProgram, "colorMod"), 0.5f, 0.5f, 0.5f, 1.0f);
    else        glUniform4f(glGetUniformLocation(shaderProgram, "colorMod"), 1.0f, 1.0f, 1.0f, 1.0f);
    float tilesPerRow   = 7.0f;
    float tileU         = (float)tileIndex / tilesPerRow;
    float tileV         = 0.0f;
    float tileUW        = 1.0f / tilesPerRow;
    float tileVH        = 1.0f; 
    glm::mat4 model     = glm::mat4(1.0f);
    float screenX       = (j - i) * (TILE_WIDTH / 2.0f);
    float screenY       = (i + j) * (TILE_HEIGHT / 2.0f);
    model               = glm::translate(model, glm::vec3(screenX + SCREEN_WIDTH / 2 - TILE_WIDTH / 2, screenY + SCREEN_HEIGHT / 2 - (mapRows * TILE_HEIGHT) / 2, 0.0f));
    model               = glm::scale(model, glm::vec3(TILE_WIDTH, TILE_HEIGHT, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    float centerU = tileU + tileUW / 2.0f;
    float centerV = tileV + tileVH / 2.0f;
    float vertices[] = {
        0.5f, 0.5f, centerU,        centerV,
        0.5f, 0.0f, centerU,        tileV,
        1.0f, 0.5f, tileU + tileUW, centerV,
        0.5f, 1.0f, centerU,        tileV + tileVH,
        0.0f, 0.5f, tileU,          centerV,
        0.5f, 0.0f, centerU,        tileV
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 6);
}

// --- COIN DRAWING FUNCTION ---
void drawCoin(int i, int j, glm::mat4 projection) {
    int winWidth, winHeight;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &winWidth, &winHeight);
    float mapHeight = (mapRows + mapCols) * (TILE_HEIGHT / 2.0f);
    float offsetY   = winHeight / 2 - mapHeight / 2;
    glUniform4f(glGetUniformLocation(shaderProgram, "colorMod"), 1.0f, 1.0f, 1.0f, 1.0f);
    glm::mat4 model = glm::mat4(1.0f);
    float screenX   = (j - i) * (TILE_WIDTH / 2.0f);
    float screenY   = (i + j) * (TILE_HEIGHT / 2.0f);
    float px        = screenX + winWidth / 2 - TILE_WIDTH / 2;
    float py        = screenY + offsetY;
    float coinW     = TILE_WIDTH * 0.25f;
    float coinH     = TILE_HEIGHT * 0.35f;
    model           = glm::translate(model, glm::vec3(px + (TILE_WIDTH - coinW) / 2, py + (TILE_HEIGHT - coinH) / 2, 0.0f));
    model           = glm::scale(model, glm::vec3(coinW, coinH, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, coinTextures[coinFrame]);
    glUniform1i(glGetUniformLocation(shaderProgram, "tileset"), 0);
    float vertices[] = {
        0.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f, 1.0f,
        1.0f, 0.0f, 1.0f, 0.0f,
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

// --- GAME RESET FUNCTION ---
void resetGame() {
    loadMapFromFile("../assets/map.txt");
    playerX     = (int)std::floor(mapCols / 2.0);
    playerY     = (int)std::floor(mapRows / 2.0);
    if (playerY >= 0 && playerY < mapRows && playerX >= 0 && playerX < mapCols) {
        mapData[playerY][playerX].tileIndex = 6;
        mapData[playerY][playerX].hasCoin   = false;
    }
    coinCount   = 0;
    totalCoins  = 0;
    for (int i = 0; i < mapRows; ++i) for (int j = 0; j < mapCols; ++j) if (mapData[i][j].hasCoin) totalCoins++;
    gameState = RUNNING;
    startTime = glfwGetTime();
}

// --- INPUT PROCESSING FUNCTION ---
void processInput(GLFWwindow* window) {
    static double lastMove  = 0;
    double now              = glfwGetTime();
    if (gameState != RUNNING) {
        if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
            resetGame();
            printf("--- Jogo iniciado! ---\n");
            printf("Colete todas as moedas, sem pisar na lava!\n");
        }
        return;
    }
    if (now - lastMove < 0.20) return; 
    int dx = 0, dy = 0;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)      dy--;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)    dy++;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)    dx--;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)   dx++;
    int nx      = playerX + dx;
    int ny      = playerY + dy;
    int rows    = (int)mapData.size();
    int cols    = rows > 0 ? (int)mapData[0].size() : 0;
    if (nx >= 0 && nx < cols && ny >= 0 && ny < rows && (dx != 0 || dy != 0)) {
        if (mapData[ny][nx].tileIndex == 5) return; 
        playerX     = nx;
        playerY     = ny;
        lastMove    = now;
        if (mapData[playerY][playerX].hasCoin) {
            mapData[playerY][playerX].hasCoin = false;
            coinCount++;
        }
        if (coinCount == totalCoins) {
            gameState = WON;
            double elapsed = now - startTime;
            printf("Parabens, voce ganhou o jogo em %.1f segundos!\n", elapsed);
            printf("Pressione R para reiniciar.\n\n");
        }
        if (mapData[playerY][playerX].tileIndex == 3) {
            gameState = GAMEOVER;
            printf("Game over, voce pisou na lava!\n");
            printf("Pressione R para reiniciar.\n\n");
        }
    }
}

// --- PLAYER DRAWING FUNCTION ---
void drawPlayer(int i, int j, glm::mat4 projection) {
    int winWidth, winHeight;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &winWidth, &winHeight);
    float mapHeight = (mapRows + mapCols) * (TILE_HEIGHT / 2.0f);
    float offsetY   = winHeight / 2 - mapHeight / 2;
    glUniform4f(glGetUniformLocation(shaderProgram, "colorMod"), 1.0f, 1.0f, 1.0f, 1.0f);
    float screenX   = (j - i) * (TILE_WIDTH / 2.0f);
    float screenY   = (i + j) * (TILE_HEIGHT / 2.0f);
    float px        = screenX + winWidth / 2 - TILE_WIDTH / 2;
    float py        = screenY + offsetY;
    float spriteW   = TILE_WIDTH * 0.7f; 
    float spriteH   = TILE_HEIGHT * 1.2f; 
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(px + (TILE_WIDTH - spriteW) / 2, py + (TILE_HEIGHT - spriteH) - TILE_HEIGHT / 4, 0.0f));
    model = glm::scale(model, glm::vec3(spriteW, spriteH, 1.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    float fw = 1.0f / 4.0f, fh = 1.0f; 
    float u0 = playerIdleFrame * fw;
    float v0 = 0.0f;
    float u1 = u0 + fw;
    float v1 = v0 + fh;
    float vertices[] = {
        0.0f, 0.0f, u0, v0,
        0.0f, 1.0f, u0, v1,
        1.0f, 1.0f, u1, v1,
        1.0f, 0.0f, u1, v0,
    };
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, playerIdleTexture);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
}

// --- MAIN GAME LOOP ---
int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Tilemap Isometrico", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    shaderProgram = createShaderProgram();
    glUseProgram(shaderProgram);
    resetGame();
    printf("--- Jogo iniciado! ---\n");
    printf("Colete todas as moedas, sem pisar na lava!\n");
    loadTileset(std::string("../assets/tilesets/") + tilesetFile);
    loadPlayerTexture("../assets/sprites/Vampirinho.png");
    loadCoinTextures();
    loadPlayerIdleTexture();
    initBuffers();
    projection = glm::ortho(0.0f, (float)SCREEN_WIDTH, (float)SCREEN_HEIGHT, 0.0f);
    double lastTime = glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        double now      = glfwGetTime();
        double delta    = now - lastTime;
        lastTime        = now;
        coinAnimTimer += delta;
        if (coinAnimTimer > coinAnimSpeed) {
            coinAnimTimer   = 0;
            coinFrame       = (coinFrame + 1) % 10;
        }
        playerIdleTimer += delta;
        if (playerIdleTimer > playerIdleSpeed) {
            playerIdleTimer = 0;
            playerIdleFrame = (playerIdleFrame + 1) % 4;
        }
        processInput(window);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindTexture(GL_TEXTURE_2D, tilesetTexture);
        for (int i = 0; i < mapRows; ++i) for (int j = 0; j < mapCols; ++j) {
            bool darken = (playerY == i && playerX == j);
            drawTile(mapData[i][j].tileIndex, i, j, projection, darken);
            if (mapData[i][j].hasCoin) drawCoin(i, j, projection);
        }
        drawPlayer(playerY, playerX, projection);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    printf("------------------------------------------\n");
    glfwTerminate();
    return 0;
}