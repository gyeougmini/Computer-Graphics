#include <iostream>
#include <fstream>
#include <string>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint shaderProgramID;
GLuint cubevao, cubevbo[2];
GLuint pyramidvao, pyramidvbo[2];
GLuint axisvao, axisvbo[2];

bool depth = false;
float xPos = 0.0f, yPos = 0.0f, zPos = 0.0f;

bool isWireframe = false;

bool rotateX = false;
float xAngle = 0.0f;
float xSpeed = 0.0f; // 초당 회전 각도 스케일용

bool rotateY = false;
float yAngle = 0.0f;
float ySpeed = 0.0f; // 초당 회전 각도 스케일용


int randomCubeFaces[2] = { -1, -1 };
int randomPyramidFace = -1;

int drawCubeFace = -1;
int drawPyramidFace = -2;

std::string filetobuf(const char* file) {
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
}

void InitShader() {
    std::string vStr = filetobuf("vertex14.glsl");
    std::string fStr = filetobuf("fragment14.glsl");
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
        {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
        { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f},
        {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},
        { 0.5f, 0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f},
        {-0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
        { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
        { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f,-0.5f},
        { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f},
        { 0.5f, 0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f},
        {-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
        {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}
    };

    GLfloat pyramid[18][3] = {
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
        { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f,-0.5f},
        { 0.5f, -0.5f, 0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.0f, 0.5f, 0.0f},
        {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}, { 0.0f, 0.5f, 0.0f},
        {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, { 0.0f, 0.5f, 0.0f},
        {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, { 0.0f, 0.5f, 0.0f}
    };

    GLfloat cubeColors[36][3];
    for (int i = 0; i < 36; i++) {
        int face = i / 6;
        if (face == 0) { cubeColors[i][0] = 1.0f; cubeColors[i][1] = 0.0f; cubeColors[i][2] = 0.0f; }
        else if (face == 1) { cubeColors[i][0] = 0.0f; cubeColors[i][1] = 1.0f; cubeColors[i][2] = 0.0f; }
        else if (face == 2) { cubeColors[i][0] = 0.0f; cubeColors[i][1] = 0.0f; cubeColors[i][2] = 1.0f; }
        else if (face == 3) { cubeColors[i][0] = 1.0f; cubeColors[i][1] = 1.0f; cubeColors[i][2] = 0.0f; }
        else if (face == 4) { cubeColors[i][0] = 1.0f; cubeColors[i][1] = 0.0f; cubeColors[i][2] = 1.0f; }
        else { cubeColors[i][0] = 0.0f; cubeColors[i][1] = 1.0f; cubeColors[i][2] = 1.0f; }
    }

    GLfloat pyramidColors[18][3];
    for (int i = 0; i < 18; i++) {
        if (i < 6) {
            pyramidColors[i][0] = 0.5f; pyramidColors[i][1] = 0.5f; pyramidColors[i][2] = 0.5f;
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

    bool isShift = (mods & GLFW_MOD_SHIFT) != 0;

    switch (key) {
    case GLFW_KEY_Q:
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

    case GLFW_KEY_C: drawCubeFace = -1; drawPyramidFace = -2; break;
    case GLFW_KEY_P: drawPyramidFace = -1; drawCubeFace = -2; break;
    case GLFW_KEY_H:
        depth = !depth;
        break;

    case GLFW_KEY_UP:    yPos += 0.05f; break;
    case GLFW_KEY_DOWN:  yPos -= 0.05f; break;
    case GLFW_KEY_RIGHT: xPos += 0.05f; break;
    case GLFW_KEY_LEFT:  xPos -= 0.05f; break;

    case GLFW_KEY_S:
        xPos = 0.0f; yPos = 0.0f; zPos = 0.0f;
        rotateX = false; xAngle = 0.0f; xSpeed = 0.0f;
        rotateY = false; yAngle = 0.0f; ySpeed = 0.0f;
        break;

    case GLFW_KEY_W:
        isWireframe = !isWireframe;
        break;

    case GLFW_KEY_X:
        rotateX = true;
        xSpeed = isShift ? -60.0f : 60.0f; // 초당 60도 기준
        break;

    case GLFW_KEY_Y:
        rotateY = true;
        ySpeed = isShift ? -60.0f : 60.0f; // 초당 60도 기준
        break;
    }
}

void drawScene(GLFWwindow* window, float deltaTime) {
    // 델타 타임을 반영한 회전각 계산 (초당 xSpeed / ySpeed 만큼 회전)
    if (rotateX) {
        xAngle += xSpeed * deltaTime;
        if (xAngle >= 360.0f) xAngle -= 360.0f;
        if (xAngle < 0.0f) xAngle += 360.0f;
    }

    if (rotateY) {
        yAngle += ySpeed * deltaTime;
        if (yAngle >= 360.0f) yAngle -= 360.0f;
        if (yAngle < 0.0f) yAngle += 360.0f;
    }

    if (!depth) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    glm::mat4 view = glm::lookAt(
        glm::vec3(2.0f, 2.0f, 2.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 600.0f / 600.0f, 0.1f, 100.0f);
    GLuint transformLoc = glGetUniformLocation(shaderProgramID, "transform");

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glm::mat4 axisModel = glm::mat4(1.0f);
    glm::mat4 axisMVP = projection * view * axisModel;
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(axisMVP));

    glBindVertexArray(axisvao);
    glLineWidth(1.0f);
    glDrawArrays(GL_LINES, 0, 6);

    if (isWireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    glm::mat4 objectModel = glm::mat4(1.0f);
    objectModel = glm::translate(objectModel, glm::vec3(xPos, yPos, zPos));
    objectModel = glm::rotate(objectModel, glm::radians(60.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    objectModel = glm::rotate(objectModel, glm::radians(xAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    objectModel = glm::rotate(objectModel, glm::radians(yAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 objectMVP = projection * view * objectModel;
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(objectMVP));

    if (drawCubeFace == -1) {
        glBindVertexArray(cubevao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    if (drawPyramidFace == -1) {
        glBindVertexArray(pyramidvao);
        glDrawArrays(GL_TRIANGLES, 0, 18);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(600, 600, "OpenGL Triangle Example", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    InitShader();
    InitBuffer();

    glfwSetKeyCallback(window, key_callback);

    float lastTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        // 프레임 드랍이나 순간 튐 현상 방지용 안전 장치
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        drawScene(window, deltaTime);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}