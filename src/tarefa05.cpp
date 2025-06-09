#define STB_IMAGE_IMPLEMENTATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"
#include <iostream>

// --- SHADER SOURCES ---
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    layout (location = 1) in vec2 aTexCoord;
    out vec2 TexCoord;
    uniform float offset;
    uniform float scale;
    void main() {
        vec3 pos = aPos * scale;
        gl_Position = vec4(pos, 1.0);
        TexCoord = vec2(aTexCoord.x - offset, aTexCoord.y);
    }
)";
const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    in vec2 TexCoord;
    uniform sampler2D texture1;
    void main() { FragColor = texture(texture1, TexCoord); }
)";

// --- LAYER STRUCT ---
struct Layer {
    unsigned int textureID;
    float speed;
    float offset;
};

// --- WINDOW SIZE CONSTANTS ---
const unsigned int SCR_WIDTH  = 800;
const unsigned int SCR_HEIGHT = 600;

// --- TEXTURE LOADING FUNCTION ---
unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); 
    unsigned char *data = stbi_load(path, &width, &height, &nrChannels, STBI_rgb_alpha);
    if (data) {
        GLenum format = GL_RGBA;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    } else {
        std::cout << "failed to load texture " << path << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

int main() {
    // --- GLFW/GLAD/OPENGL INIT ---
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "parallax", NULL, NULL);
    if (window == NULL) {
        std::cout << "failed to create glfw window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    // --- BLENDING/SHADER/VAO/VBO SETUP ---
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f, 
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f, 
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f, 
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f, 
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f
    };
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

    // --- LAYER SETUP ---
    const char* layerPaths[6] = {
        "../assets/layers/sky_pale.png",
        "../assets/layers/houses3_pale.png",
        "../assets/layers/houded2_pale.png",
        "../assets/layers/houses1_pale.png",
        "../assets/layers/crosswalk_pale.png",
        "../assets/layers/road_pale.png"
    };
    float minSpeed = 0.05f;
    float maxSpeed = 1.50f;
    float scale    = 1.0f;
    Layer layers[6];
    for (int i = 0; i < 6; ++i) {
        layers[i].textureID = loadTexture(layerPaths[i]);
        layers[i].speed     = minSpeed + (maxSpeed - minSpeed) * (float)i / 5.0f;
        layers[i].offset    = 0.0f;
    }

    // --- SPRITE TEXTURE/FRAME CONSTANTS ---
    unsigned int idleTexture   = loadTexture("../assets/sprites/Idle.png");
    unsigned int walkTexture   = loadTexture("../assets/sprites/Walk.png");
    unsigned int jumpTexture   = loadTexture("../assets/sprites/Jump.png");
    unsigned int attackTexture = loadTexture("../assets/sprites/Attack_2.png");
    unsigned int runTexture    = loadTexture("../assets/sprites/Run.png");
    int currentFrame = 0;
    int idleFrames   = 6;
    int walkFrames   = 8;
    int jumpFrames   = 12;
    int attackFrames = 4;
    int runFrames    = 8;

    // --- ANIMATION/CHARACTER STATE VARS ---
    bool walking        = false;
    bool attacking      = false;
    bool jumping        = false;
    bool isJumping      = false;
    float frameDuration = 0.15f;
    float frameTimer    = 0.0f;
    float attackTimer   = 0.0f;
    float charX         = 0.0f;
    float charY         = -0.5f;
    float charW         = 0.2f;
    float charH         = 0.4f;
    float jumpVelocity  = 1.2f;
    float gravity       = -2.5f;
    float jumpY         = 0.0f;
    float jumpSpeed     = 0.0f;
    float lastTime      = glfwGetTime();
    float attackDuration = attackFrames * frameDuration;
    float charVertices[30];

    // --- CHARACTER VAO/VBO SETUP ---
    unsigned int charVBO, charVAO;
    glGenVertexArrays(1, &charVAO);
    glGenBuffers(1, &charVBO);
    glBindVertexArray(charVAO);
    glBindBuffer(GL_ARRAY_BUFFER, charVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(charVertices), charVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // --- MAIN GAME LOOP ---
    while (!glfwWindowShouldClose(window)) {
        float now   = glfwGetTime();
        float delta = now - lastTime;
        lastTime    = now;
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);

        // --- INPUT HANDLING ---
        bool leftPressed   = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
        bool rightPressed  = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
        bool jumpPressed   = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
        bool attackPressed = glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
        bool running       = (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) && (leftPressed || rightPressed);

        // --- MOVEMENT LOGIC ---
        float speedMultiplier = running ? 2.0f : 1.0f;
        float baseSpeed       = 0.0001f;
        bool isMoving         = rightPressed || leftPressed;
        float moveDir         = rightPressed ? -1.0f : (leftPressed ? 1.0f : 0.0f);
        if (isMoving) {
            float moveSpeed = baseSpeed * speedMultiplier * moveDir;
            for (int i = 0; i < 6; ++i) layers[i].offset += layers[i].speed * moveSpeed;
        }
        walking = isMoving;
        jumping = jumpPressed;

        // --- JUMP LOGIC ---
        if (!isJumping && jumpPressed) {
            isJumping = true;
            jumpSpeed = jumpVelocity;
        }
        if (isJumping) {
            jumpY     += jumpSpeed * delta;
            jumpSpeed += gravity * delta;
            if (jumpY <= 0.0f) {
                jumpY     = 0.0f;
                isJumping = false;
                jumpSpeed = 0.0f;
            }
        }

        // --- FLIP SPRITE ---
        static bool lastFlip       = false;
        if (leftPressed) lastFlip  = true;
        if (rightPressed) lastFlip = false;
        bool flip = lastFlip;

        // --- ATTACK LOGIC ---
        static bool lastAttackPressed = false;
        if (attackPressed && !lastAttackPressed && !attacking) {
            attacking    = true;
            attackTimer  = 0.0f;
            currentFrame = 0;
        }
        lastAttackPressed = attackPressed;
        if (attacking) {
            attackTimer += delta;
            if (attackTimer >= attackDuration) {
                attacking   = false;
                attackTimer = 0.0f;
            }
        }

        // --- ANIMATION STATE ---
        int totalFrames;
        unsigned int charTex;
        if (attacking) {
            totalFrames  = attackFrames;
            charTex      = attackTexture;
            currentFrame = (int)((attackTimer / attackDuration) * attackFrames);
            if (currentFrame >= attackFrames) currentFrame = attackFrames - 1;
        } else if (isJumping || jumping) {
            totalFrames = jumpFrames;
            charTex     = jumpTexture;
        } else if (running) {
            totalFrames = runFrames;
            charTex     = runTexture;
        } else if (walking) {
            totalFrames = walkFrames;
            charTex     = walkTexture;
        } else {
            totalFrames = idleFrames;
            charTex     = idleTexture;
        }
        if (!attacking) {
            frameTimer += delta;
            if (frameTimer >= frameDuration) {
                frameTimer   = 0.0f;
                currentFrame = (currentFrame + 1) % totalFrames;
            }
        }

        // --- SPRITE UVs ---
        float u0 = (float)currentFrame / totalFrames;
        float u1 = (float)(currentFrame + 1) / totalFrames;
        if (flip) {
            float tmp = u0;
            u0        = u1;
            u1        = tmp;
        }

        // --- CHARACTER VERTEX UPDATE ---
        float charYdraw  = charY + jumpY;
        charVertices[0]  = charX - charW/2; charVertices[1]  = charYdraw + charH/2; charVertices[2]  = 0.0f; charVertices[3]  = u0; charVertices[4]  = 1.0f;
        charVertices[5]  = charX - charW/2; charVertices[6]  = charYdraw - charH/2; charVertices[7]  = 0.0f; charVertices[8]  = u0; charVertices[9]  = 0.0f;
        charVertices[10] = charX + charW/2; charVertices[11] = charYdraw - charH/2; charVertices[12] = 0.0f; charVertices[13] = u1; charVertices[14] = 0.0f;
        charVertices[15] = charX - charW/2; charVertices[16] = charYdraw + charH/2; charVertices[17] = 0.0f; charVertices[18] = u0; charVertices[19] = 1.0f;
        charVertices[20] = charX + charW/2; charVertices[21] = charYdraw - charH/2; charVertices[22] = 0.0f; charVertices[23] = u1; charVertices[24] = 0.0f;
        charVertices[25] = charX + charW/2; charVertices[26] = charYdraw + charH/2; charVertices[27] = 0.0f; charVertices[28] = u1; charVertices[29] = 1.0f;

        // --- DRAW SCENE ---
        glBindBuffer(GL_ARRAY_BUFFER, charVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(charVertices), charVertices, GL_DYNAMIC_DRAW);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(VAO);
        glUseProgram(shaderProgram);

        // --- DRAW LAYERS ---
        for (int i = 0; i < 6; ++i) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, layers[i].textureID);
            glUniform1f(glGetUniformLocation(shaderProgram, "offset"), layers[i].offset);
            glUniform1f(glGetUniformLocation(shaderProgram, "scale"), scale);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        // --- DRAW CHARACTER ---
        glBindVertexArray(charVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, charTex);
        glUniform1f(glGetUniformLocation(shaderProgram, "offset"), 0.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "scale"), 1.0f);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // --- CLEANUP ---
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &charVAO);
    glDeleteBuffers(1, &charVBO);
    glfwTerminate();
    return 0;
}