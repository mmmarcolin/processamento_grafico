#include <iostream>
#include <string>
#include <vector>
#include <assert.h>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
using namespace std;
const GLuint WIDTH = 800, HEIGHT = 600;

const GLchar* vertexShaderSource = R"(
    #version 400
    layout (location = 0) in vec3 position;
    layout (location = 1) in vec2 texCoord;
    uniform mat4 model;
    uniform mat4 projection;
    out vec2 TexCoord;
    void main() {
        TexCoord = texCoord;
        gl_Position = projection * model * vec4(position, 1.0);
    }
)";

const GLchar* fragmentShaderSource = R"(
    #version 400
    in vec2 TexCoord;
    out vec4 color;
    uniform sampler2D tex;
    void main() { color = texture(tex, TexCoord); }
)";

glm::mat4 projection = glm::ortho(0.0f, float(WIDTH), 0.0f, float(HEIGHT), -1.0f, 1.0f);
class Sprite {
public:
    GLuint VAO;
    GLuint textureID;
    GLuint shaderProgram;
    glm::vec2 position, scale;
    float rotation;
    Sprite(GLuint shader, GLuint texID, glm::vec2 pos, glm::vec2 scl, float rot)
        : shaderProgram(shader), textureID(texID), position(pos), scale(scl), rotation(rot) {
        setupVAO();
    }
    void draw() {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f));
        model = glm::rotate(model, glm::radians(rotation), glm::vec3(0, 0, 1));
        model = glm::scale(model, glm::vec3(scale, 1.0f));
        glUseProgram(shaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glBindTexture(GL_TEXTURE_2D, textureID);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }
private:
    void setupVAO() {
        GLfloat vertices[] = {
            -0.5f,  0.5f, 0.0f,  0.0f, 1.0f,
            -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
             0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
             0.5f, -0.5f, 0.0f,  1.0f, 0.0f
        };
        GLuint VBO;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
};

GLuint compileShader(GLenum type, const GLchar* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Erro na compilação do shader: " << infoLog << std::endl;
    }
    return shader;
}

GLuint createShaderProgram() {
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "Erro ao linkar o programa: " << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

GLuint loadTexture(const string& path) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else { std::cerr << "Falha ao carregar textura: " << path << std::endl; }
    stbi_image_free(data);
    return textureID;
}

int main() {
    glfwInit();
    stbi_set_flip_vertically_on_load(true);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Sprites com Textura", nullptr, nullptr);
    if (!window) {
        std::cerr << "Erro ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Erro ao inicializar GLAD" << std::endl;
        return -1;
    }
    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLuint shaderProgram = createShaderProgram();
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);
    vector<Sprite> sprites;
    vector<string> texturePaths = {
        "../assets/sprites/night.png",
        "../assets/sprites/3_Run_005.png",
        "../assets/sprites/2_Jump_001.png",
        "../assets/sprites/1_Dying_014.png",
        "../assets/sprites/Protect.png"
    };
    for (int i = 0; i < texturePaths.size(); i++) {
        GLuint tex = loadTexture(texturePaths[i]);
        if (i == 0) { sprites.emplace_back(shaderProgram, tex, glm::vec2(WIDTH / 2.0f, HEIGHT / 2.0f), glm::vec2(WIDTH, HEIGHT), 0.0f); } 
        else {
            float x = 100.0f + (i - 1) * 140.0f;
            sprites.emplace_back(shaderProgram, tex, glm::vec2(x, 100), glm::vec2(128.0f, 128.0f), 0.0f);
        }
    }
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.3f, 0.4f, 0.6f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        for (auto& sprite : sprites) { sprite.draw(); }
        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return 0;
}
