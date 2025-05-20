#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;
using namespace glm;

const GLint WIDTH = 1080, HEIGHT = 720;
GLuint shaderID;
GLuint colorLoc;
GLuint projectionLoc;
mat4 projection;
struct Triangle {
    vec2 position;
    vec4 color;
};
std::vector<Triangle> triangles;
std::vector<GLuint> staticVAOs;
GLuint baseVAO;

const char* vertex_shader = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    uniform mat4 projection;
    uniform mat4 model;
    void main() {
        gl_Position = projection * model * vec4(aPos, 1.0);
    }
)";

const char* fragment_shader = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec4 color;
    void main() { 
        FragColor = color;
    }
)";

GLuint createTriangle(float x0, float y0, float x1, float y1, float x2, float y2) {
    float vertices[] = {
        x0, y0, 0.0f,
        x1, y1, 0.0f,
        x2, y2, 0.0f
    };
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    return VAO;
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        float x = static_cast<float>(xpos);
        float y = static_cast<float>(HEIGHT - ypos);
        float r = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.8f;
        float g = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.8f;
        float b = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.8f;
        Triangle t;
        t.position = vec2(x, y);
        t.color = vec4(r, g, b, 1.0f);
        triangles.push_back(t);
    }
}

int main() {
    srand(static_cast<unsigned int>(time(0)));
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Triângulos coloridos", NULL, NULL);
    if (!window) {
        cout << "Erro criando janela" << endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glViewport(0, 0, WIDTH, HEIGHT);
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertexShader, 1, &vertex_shader, NULL);
    glCompileShader(vertexShader);
    glShaderSource(fragmentShader, 1, &fragment_shader, NULL);
    glCompileShader(fragmentShader);
    shaderID = glCreateProgram();
    glAttachShader(shaderID, vertexShader);
    glAttachShader(shaderID, fragmentShader);
    glLinkProgram(shaderID);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    glUseProgram(shaderID);
    colorLoc = glGetUniformLocation(shaderID, "color");
    projectionLoc = glGetUniformLocation(shaderID, "projection");
    projection = ortho(0.0f, static_cast<float>(WIDTH), 0.0f, static_cast<float>(HEIGHT));
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, value_ptr(projection));
    staticVAOs.push_back(createTriangle(100, 100, 150, 100, 125, 150));
    staticVAOs.push_back(createTriangle(300, 200, 350, 200, 325, 250));
    staticVAOs.push_back(createTriangle(500, 400, 550, 400, 525, 450));
    staticVAOs.push_back(createTriangle(700, 100, 750, 100, 725, 150));
    staticVAOs.push_back(createTriangle(900, 300, 950, 300, 925, 350));
    baseVAO = createTriangle(-0.1f * WIDTH, -0.1f * HEIGHT, 0.1f * WIDTH, -0.1f * HEIGHT, 0.0f, 0.1f * HEIGHT);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderID);
        for (GLuint vao : staticVAOs) {
            glBindVertexArray(vao);
            glUniform4f(colorLoc, 0.8f, 0.3f, 0.2f, 1.0f);
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(mat4(1.0f)));
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        for (const Triangle& t : triangles) {
            mat4 model = translate(mat4(1.0f), vec3(t.position, 0.0f));
            glBindVertexArray(baseVAO);
            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, value_ptr(model));
            glUniform4f(colorLoc, t.color.r, t.color.g, t.color.b, t.color.a);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return EXIT_SUCCESS;
}
