#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const GLint WIDTH = 800, HEIGHT = 600;
GLuint VAO, VBO;
GLuint shader_programme;
std::vector<GLfloat> vertices;
std::vector<glm::vec2> points;
std::vector<glm::vec2> temp_points;
glm::mat4 proj;

const char* vertex_shader = R"(
    #version 410 core
    layout(location = 0) in vec3 position;
    layout(location = 1) in vec3 color;
    uniform mat4 proj;
    out vec3 fragColor;
    void main() { 
        fragColor = color;
        gl_Position = proj * vec4(position, 1.0);
    };
)";

const char* fragment_shader = R"(
    #version 410 core
    in vec3 fragColor;
    out vec4 color;
    void main() { color = vec4(fragColor, 1.0); };
)";

void mouse_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);
        temp_points.push_back(glm::vec2(mx, my));
        if (temp_points.size() >= 3) {
            glm::vec3 color(
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX,
                static_cast<float>(rand()) / RAND_MAX
            );
            for(const auto& p : temp_points) {
                vertices.insert(vertices.end(), { p.x, p.y, 0.0f, color.r, color.g, color.b });
            }
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GLfloat), vertices.data(), GL_DYNAMIC_DRAW);
            temp_points.clear();
        }
    }
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Triângulos coloridos", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetMouseButtonCallback(window, mouse_callback);
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertex_shader, NULL);
    glCompileShader(vs);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragment_shader, NULL);
    glCompileShader(fs);
    shader_programme = glCreateProgram();
    glAttachShader(shader_programme, vs);
    glAttachShader(shader_programme, fs);
    glLinkProgram(shader_programme);
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    proj = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f, -1.0f, 1.0f);
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shader_programme);
        glUniformMatrix4fv(glGetUniformLocation(shader_programme, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
        glEnableVertexAttribArray(1);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(vertices.size() / 6));
        glfwSwapBuffers(window);
    }
    glfwTerminate();
    return EXIT_SUCCESS;
}