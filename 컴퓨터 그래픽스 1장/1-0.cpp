#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint shaderProgramID;
GLuint cubevao, cubevbo[2];
GLuint pyramidvao, pyramidvbo[2];
GLuint axisvao, axisvbo[2];



std::string filetobuf(const char* file) {
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
}

void InitShader() {
    std::string vStr = filetobuf("vertex16.glsl");
    std::string fStr = filetobuf("fragment16.glsl");
    const char* vSrc = vStr.c_str();
    const char* fSrc = fStr.c_str();

    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vSrc, NULL);
    glCompileShader(vShader);

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fSrc, NULL);
    glCompileShader(fShader);

    shaderProgramID = glCreateProgram();
    glAttachShader(shaderProgramID, vShader);
    glAttachShader(shaderProgramID, fShader);
    glLinkProgram(shaderProgramID);

    glDeleteShader(vShader);
    glDeleteShader(fShader);
}

void InitBuffer() {

    GLfloat cubeShape[36][3] = {
        // 앞면 (0)
        {-0.8f,-0.2f, 0.2f}, {-0.4f,-0.2f, 0.2f}, {-0.4f, 0.2f, 0.2f},
        {-0.4f, 0.2f, 0.2f}, {-0.8f, 0.2f, 0.2f}, {-0.8f,-0.2f, 0.2f},
        // 뒷면 (1)
        {-0.8f,-0.2f,-0.2f}, {-0.4f,-0.2f,-0.2f}, {-0.4f, 0.2f,-0.2f},
        {-0.4f, 0.2f,-0.2f}, {-0.8f, 0.2f,-0.2f}, {-0.8f,-0.2f,-0.2f},
        // 윗면 (2)
        {-0.8f, 0.2f,-0.2f}, {-0.8f, 0.2f, 0.2f}, {-0.4f, 0.2f, 0.2f},
        {-0.4f, 0.2f, 0.2f}, {-0.4f, 0.2f,-0.2f}, {-0.8f, 0.2f,-0.2f},
        // 밑면 (3)
        {-0.8f,-0.2f,-0.2f}, {-0.8f,-0.2f, 0.2f}, {-0.4f,-0.2f, 0.2f},
        {-0.4f,-0.2f, 0.2f}, {-0.4f,-0.2f,-0.2f}, {-0.8f,-0.2f,-0.2f},
        // 오른면 (4)
        {-0.4f,-0.2f,-0.2f}, {-0.4f, 0.2f,-0.2f}, {-0.4f, 0.2f, 0.2f},
        {-0.4f, 0.2f, 0.2f}, {-0.4f,-0.2f, 0.2f}, {-0.4f,-0.2f,-0.2f},
        // 왼면 (5)
        {-0.8f,-0.2f,-0.2f}, {-0.8f, 0.2f,-0.2f}, {-0.8f, 0.2f, 0.2f},
        {-0.8f, 0.2f, 0.2f}, {-0.8f,-0.2f, 0.2f}, {-0.8f,-0.2f,-0.2f}
    };

    GLfloat cubeColors[36][3];
    for (int i = 0; i < 36; i++) {
        int face = i / 6;
        if (face == 0) { cubeColors[i][0] = 1.0f; cubeColors[i][1] = 0.0f; cubeColors[i][2] = 0.0f; } // 빨강
        else if (face == 1) { cubeColors[i][0] = 0.0f; cubeColors[i][1] = 1.0f; cubeColors[i][2] = 0.0f; } // 초록
        else if (face == 2) { cubeColors[i][0] = 0.0f; cubeColors[i][1] = 0.0f; cubeColors[i][2] = 1.0f; } // 파랑
        else if (face == 3) { cubeColors[i][0] = 1.0f; cubeColors[i][1] = 1.0f; cubeColors[i][2] = 0.0f; } // 노랑
        else if (face == 4) { cubeColors[i][0] = 1.0f; cubeColors[i][1] = 0.0f; cubeColors[i][2] = 1.0f; } // 자홍
        else { cubeColors[i][0] = 0.0f; cubeColors[i][1] = 1.0f; cubeColors[i][2] = 1.0f; } // 청록
    }


    GLfloat pyramid[18][3] = {
        // 아랫면 (0)
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
        { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f,-0.5f},
        // 앞면 (1)
        { 0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, { 0.0f, 0.5f, 0.0f},
        // 오른면 (2)
        { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, 0.5f}, { 0.0f, 0.5f, 0.0f},
        // 뒷면 (3)
        {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.0f, 0.5f, 0.0f},
        // 왼면 (4)
        {-0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, -0.5f}, { 0.0f, 0.5f, 0.0f}
    };

    GLfloat pyramidColors[18][3];
    for (int i = 0; i < 18; i++) {
        if (i < 6) {
            pyramidColors[i][0] = 0.5f; pyramidColors[i][1] = 0.5f; pyramidColors[i][2] = 0.5f; // 회색
        }
        else {
            int sideFace = (i - 6) / 3;
            if (sideFace == 0) { pyramidColors[i][0] = 1.0f; pyramidColors[i][1] = 0.0f; pyramidColors[i][2] = 0.0f; }
            else if (sideFace == 1) { pyramidColors[i][0] = 0.0f; pyramidColors[i][1] = 1.0f; pyramidColors[i][2] = 0.0f; }
            else if (sideFace == 2) { pyramidColors[i][0] = 0.0f; pyramidColors[i][1] = 0.0f; pyramidColors[i][2] = 1.0f; }
            else { pyramidColors[i][0] = 1.0f; pyramidColors[i][1] = 1.0f; pyramidColors[i][2] = 0.0f; }
        }
    }

    GLfloat axisShape[6][3] = {
        {-2.0f, 0.0f, 0.0f}, { 2.0f, 0.0f, 0.0f},
        { 0.0f,-2.0f, 0.0f}, { 0.0f, 2.0f, 0.0f},
        { 0.0f, 0.0f,-2.0f}, { 0.0f, 0.0f, 2.0f}
    };

    GLfloat axisColors[6][3] = {
        {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}
    };

    // 정육면체 VBO
    glGenVertexArrays(1, &cubevao);
    glBindVertexArray(cubevao);
    glGenBuffers(2, cubevbo);

    glBindBuffer(GL_ARRAY_BUFFER, cubevbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeShape), cubeShape, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, cubevbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeColors), cubeColors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    // 피라미드 VBO
    glGenVertexArrays(1, &pyramidvao);
    glBindVertexArray(pyramidvao);
    glGenBuffers(2, pyramidvbo);

    glBindBuffer(GL_ARRAY_BUFFER, pyramidvbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramid), pyramid, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, pyramidvbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidColors), pyramidColors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    // 축 VBO
    glGenVertexArrays(1, &axisvao);
    glBindVertexArray(axisvao);
    glGenBuffers(2, axisvbo);

    glBindBuffer(GL_ARRAY_BUFFER, axisvbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisShape), axisShape, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, axisvbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisColors), axisColors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
    case GLFW_KEY_Q:
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

   
    }
}


void drawScene(GLFWwindow* window) {

    glEnable(GL_DEPTH_TEST);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    glm::mat4 view = glm::lookAt(
        glm::vec3(2.5f, 2.0f, 2.5f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    GLuint transformLoc = glGetUniformLocation(shaderProgramID, "transform");

    // 1. 고정 축 렌더링
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glm::mat4 axisMVP = projection * view * glm::mat4(1.0f);
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(axisMVP));
    glBindVertexArray(axisvao);
    glLineWidth(1.5f);
    glDrawArrays(GL_LINES, 0, 6);

    // 2. 기본 모델 변환
    glm::mat4 baseModel = glm::mat4(1.0f);
    baseModel = glm::rotate(baseModel, glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // --- 정육면체 렌더링 ---
    glBindVertexArray(cubevao);
    glm::mat4 mvp = projection * view * baseModel;
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glDrawArrays(GL_TRIANGLES, 0, 36);
   

    // --- 피라미드 렌더링
    glBindVertexArray(pyramidvao);
    glm::mat4 baseMVP = projection * view * baseModel;
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(baseMVP));
    glDrawArrays(GL_TRIANGLES, 0, 18);
}

int main() {
    srand(static_cast<unsigned int>(time(NULL)));

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(600, 600, "Practice 15 & 16 Complete", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    InitShader();
    InitBuffer();

    glfwSetKeyCallback(window, key_callback);

    while (!glfwWindowShouldClose(window)) {
        drawScene(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}