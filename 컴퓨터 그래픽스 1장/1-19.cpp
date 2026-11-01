#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint shaderProgramID;
GLuint cubeVAO, cubeVBO[2]; // 0: 위치, 1: 색상

// --- 탱크 애니메이션 및 상태 변수 ---
glm::vec3 tankPos = glm::vec3(0.0f, 0.0f, 0.0f);
float lowerScaleY = 1.0f;
float lowerScaleDir = 1.0f;
bool animD = false;

float midRotY = 0.0f;
bool animT = false;

// L: 상부 몸체 교체 애니메이션 관련 변수
bool upperSwapped = false;
bool animSwap = false;
float swapProgress = 0.0f;

float barrelRotY = 0.0f;
float barrelDir = 1.0f;
bool animG = false;

float flagRotX = 0.0f;
float flagDir = 1.0f;
bool animP = false;

// --- 부위별 색상 변수 (기본값 설정) ---
glm::vec3 cFloor = glm::vec3(0.5f, 0.7f, 0.9f);
glm::vec3 cLower = glm::vec3(0.3f, 0.3f, 0.3f);
glm::vec3 cMid = glm::vec3(0.7f, 0.7f, 0.7f);
glm::vec3 cUpper = glm::vec3(0.4f, 0.6f, 0.3f);
glm::vec3 cBarrel = glm::vec3(0.8f, 0.5f, 0.1f);
glm::vec3 cFlag = glm::vec3(0.9f, 0.8f, 0.1f);

// --- 카메라 변수 ---
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 10.0f);
glm::vec3 cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

bool animCamY = false; float camYDir = 1.0f; float camYAngle = 0.0f; // Y/y: 카메라 자전
bool animCamR = false; float camRDir = 1.0f; float camRAngle = 0.0f; // R/r: 월드 중심 공전

// 난수 색상 생성 함수 (0.0f ~ 1.0f)
glm::vec3 getRandomColor() {
    float r = static_cast<float>(rand()) / RAND_MAX;
    float g = static_cast<float>(rand()) / RAND_MAX;
    float b = static_cast<float>(rand()) / RAND_MAX;
    return glm::vec3(r, g, b);
}

// 단위 육면체 버텍스 위치 데이터
float cubeVertices[] = {
    // Front
    -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
    // Back
    -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    // Top
    -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
    // Bottom
    -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
    // Right
     0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,
    // Left
    -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f
};

std::string filetobuf(const char* file) {
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
}

void InitShader() {
    std::string vStr = filetobuf("vertex19.glsl");
    std::string fStr = filetobuf("fragment19.glsl");
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
    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);
    glGenBuffers(2, cubeVBO);

    // Location 0: 정점 위치 (vPosition)
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // Location 1: 정점 색상 (vColor)
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, 36 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);
}

void randomizeAllColors() {
    cFloor = getRandomColor();
    cLower = getRandomColor(); // 아래 몸체 2개에 공통 적용
    cMid = getRandomColor();
    cUpper = getRandomColor(); // 상부 몸체 2개에 공통 적용
    cBarrel = getRandomColor(); // 포신 2개에 공통 적용
    cFlag = getRandomColor(); // 깃대 2개에 공통 적용
}

void resetAll() {
    tankPos = glm::vec3(0.0f);
    lowerScaleY = 1.0f; animD = false;
    midRotY = 0.0f; animT = false;
    upperSwapped = false; animSwap = false; swapProgress = 0.0f;
    barrelRotY = 0.0f; barrelDir = 1.0f; animG = false;
    flagRotX = 0.0f; flagDir = 1.0f; animP = false;

    // 색상 기본값으로 복구
    cFloor = glm::vec3(0.5f, 0.7f, 0.9f);
    cLower = glm::vec3(0.3f, 0.3f, 0.3f);
    cMid = glm::vec3(0.7f, 0.7f, 0.7f);
    cUpper = glm::vec3(0.4f, 0.6f, 0.3f);
    cBarrel = glm::vec3(0.8f, 0.5f, 0.1f);
    cFlag = glm::vec3(0.9f, 0.8f, 0.1f);

    cameraPos = glm::vec3(0.0f, 5.0f, 10.0f);
    cameraTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    animCamY = false; camYAngle = 0.0f;
    animCamR = false; camRAngle = 0.0f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    bool isShift = (mods & GLFW_MOD_SHIFT) != 0;

    switch (key) {
    case GLFW_KEY_Q: glfwSetWindowShouldClose(window, GLFW_TRUE); break;

    case GLFW_KEY_UP:    tankPos.z -= 0.1f; break;
    case GLFW_KEY_DOWN:  tankPos.z += 0.1f; break;
    case GLFW_KEY_LEFT:  tankPos.x -= 0.1f; break;
    case GLFW_KEY_RIGHT: tankPos.x += 0.1f; break;

    case GLFW_KEY_D: animD = !animD; break;
    case GLFW_KEY_T: animT = !animT; break;

    case GLFW_KEY_L:
    case GLFW_KEY_1:
        if (!animSwap) {
            animSwap = true;
            swapProgress = 0.0f;
        }
        break;

    case GLFW_KEY_G: animG = !animG; break;
    case GLFW_KEY_P: animP = !animP; break;

        // O/o 키: 누를 때마다 세트별 랜덤 색상 변경
    case GLFW_KEY_O: randomizeAllColors(); break;

    case GLFW_KEY_C: resetAll(); break;

    case GLFW_KEY_Z: cameraPos.z += isShift ? -0.2f : 0.2f; break;
    case GLFW_KEY_X: cameraPos.x += isShift ? -0.2f : 0.2f; break;

    case GLFW_KEY_Y:
        animCamY = !animCamY;
        camYDir = isShift ? -1.0f : 1.0f;
        break;

    case GLFW_KEY_R:
        animCamR = !animCamR;
        camRDir = isShift ? -1.0f : 1.0f;
        break;
    }
}

void update(float deltaTime) {
    if (animD) {
        lowerScaleY += lowerScaleDir * deltaTime * 1.0f;
        if (lowerScaleY > 2.0f) { lowerScaleY = 2.0f; lowerScaleDir = -1.0f; }
        if (lowerScaleY < 0.5f) { lowerScaleY = 0.5f; lowerScaleDir = 1.0f; }
    }
    if (animT) { midRotY += 60.0f * deltaTime; }

    if (animSwap) {
        swapProgress += deltaTime * 1.5f;
        if (swapProgress >= 1.0f) {
            swapProgress = 1.0f;
            animSwap = false;
            upperSwapped = !upperSwapped;
        }
    }

    if (animG) {
        barrelRotY += barrelDir * 90.0f * deltaTime;
        if (barrelRotY >= 60.0f) { barrelRotY = 60.0f; barrelDir = -1.0f; }
        else if (barrelRotY <= -60.0f) { barrelRotY = -60.0f; barrelDir = 1.0f; }
    }

    if (animP) {
        flagRotX += flagDir * 90.0f * deltaTime;
        if (flagRotX >= 60.0f) { flagRotX = 60.0f; flagDir = -1.0f; }
        else if (flagRotX <= -60.0f) { flagRotX = -60.0f; flagDir = 1.0f; }
    }

    if (animCamY) {
        camYAngle += camYDir * 45.0f * deltaTime;
    }

    if (animCamR) {
        camRAngle += camRDir * 45.0f * deltaTime;
    }
}

void drawCube(const glm::mat4& model, const glm::mat4& viewProj, glm::vec3 color, GLuint transformLoc) {
    glm::mat4 mvp = viewProj * model;
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    std::vector<float> colorBuffer(36 * 3);
    for (int i = 0; i < 36; ++i) {
        colorBuffer[i * 3 + 0] = color.r;
        colorBuffer[i * 3 + 1] = color.g;
        colorBuffer[i * 3 + 2] = color.b;
    }

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[1]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, colorBuffer.size() * sizeof(float), colorBuffer.data());

    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawScene(GLFWwindow* window, float deltaTime) {
    update(deltaTime);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    GLuint transformLoc = glGetUniformLocation(shaderProgramID, "transform");

    glm::vec3 currentCamPos = cameraPos;
    if (camRAngle != 0.0f || animCamR) {
        float rad = glm::radians(camRAngle);
        float x = cameraPos.x * cos(rad) + cameraPos.z * sin(rad);
        float z = -cameraPos.x * sin(rad) + cameraPos.z * cos(rad);
        currentCamPos = glm::vec3(x, cameraPos.y, z);
    }

    glm::vec3 currentTarget = cameraTarget;
    if (camYAngle != 0.0f || animCamY) {
        glm::vec3 front = glm::normalize(cameraTarget - currentCamPos);
        glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), glm::radians(camYAngle), cameraUp);
        glm::vec3 rotatedFront = glm::vec3(rotMat * glm::vec4(front, 0.0f));
        currentTarget = currentCamPos + rotatedFront;
    }

    glm::mat4 view = glm::lookAt(currentCamPos, currentTarget, cameraUp);

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 viewProj = projection * view;

    glBindVertexArray(cubeVAO);

    // 1. 바닥
    glm::mat4 floorModel = glm::mat4(1.0f);
    floorModel = glm::translate(floorModel, glm::vec3(0.0f, -0.05f, 0.0f));
    floorModel = glm::scale(floorModel, glm::vec3(12.0f, 0.1f, 12.0f));
    drawCube(floorModel, viewProj, cFloor, transformLoc);

    // 2. 탱크 기준 루트
    glm::mat4 tankRoot = glm::mat4(1.0f);
    tankRoot = glm::translate(tankRoot, tankPos);
    tankRoot = glm::rotate(tankRoot, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // 3. 아래 몸체 (2개 - 동일한 cLower 색상)
    float lowerHeight = 0.6f * lowerScaleY;
    glm::mat4 lowerLeft = glm::translate(tankRoot, glm::vec3(-0.65f, lowerHeight * 0.5f, 0.0f));
    lowerLeft = glm::scale(lowerLeft, glm::vec3(0.8f, lowerHeight, 2.0f));
    drawCube(lowerLeft, viewProj, cLower, transformLoc);

    glm::mat4 lowerRight = glm::translate(tankRoot, glm::vec3(0.65f, lowerHeight * 0.5f, 0.0f));
    lowerRight = glm::scale(lowerRight, glm::vec3(0.8f, lowerHeight, 2.0f));
    drawCube(lowerRight, viewProj, cLower, transformLoc);

    // 4. 중앙 몸체 (y축 회전 - t키)
    glm::mat4 midModel = glm::translate(tankRoot, glm::vec3(0.0f, lowerHeight + 0.15f, 0.0f));
    midModel = glm::rotate(midModel, glm::radians(midRotY), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 midDraw = glm::scale(midModel, glm::vec3(1.8f, 0.3f, 1.6f));
    drawCube(midDraw, viewProj, cMid, transformLoc);

    // 5. 상부 몸체 2개
    float startLeft = upperSwapped ? 0.5f : -0.5f;
    float startRight = upperSwapped ? -0.5f : 0.5f;

    float targetLeft = upperSwapped ? -0.5f : 0.5f;
    float targetRight = upperSwapped ? 0.5f : -0.5f;

    float currentProgress = animSwap ? swapProgress : 0.0f;
    float posX[2];
    posX[0] = (1.0f - currentProgress) * startLeft + currentProgress * targetLeft;
    posX[1] = (1.0f - currentProgress) * startRight + currentProgress * targetRight;

    for (int i = 0; i < 2; ++i) {
        // 상부 몸체 2개 (동일한 cUpper 색상)
        glm::mat4 upperModel = glm::translate(midModel, glm::vec3(posX[i], 0.35f, 0.0f));
        glm::mat4 upperDraw = glm::scale(upperModel, glm::vec3(0.6f, 0.4f, 0.8f));
        drawCube(upperDraw, viewProj, cUpper, transformLoc);

        // 포신 2개 (동일한 cBarrel 색상)
        float bRot = (i == 0) ? barrelRotY : -barrelRotY;
        glm::mat4 barrelModel = glm::translate(upperModel, glm::vec3(0.0f, 0.0f, -0.6f));
        barrelModel = glm::rotate(barrelModel, glm::radians(bRot), glm::vec3(0.0f, 1.0f, 0.0f));
        barrelModel = glm::scale(barrelModel, glm::vec3(0.12f, 0.12f, 0.6f));
        drawCube(barrelModel, viewProj, cBarrel, transformLoc);

        // 깃대 2개 (동일한 cFlag 색상)
        float fRot = (i == 0) ? flagRotX : -flagRotX;
        glm::mat4 flagModel = glm::translate(upperModel, glm::vec3(0.0f, 0.45f, 0.0f));
        flagModel = glm::rotate(flagModel, glm::radians(fRot), glm::vec3(1.0f, 0.0f, 0.0f));
        flagModel = glm::scale(flagModel, glm::vec3(0.08f, 0.5f, 0.08f));
        drawCube(flagModel, viewProj, cFlag, transformLoc);
    }
}

int main() {
    srand(static_cast<unsigned int>(time(NULL))); // 시드 초기화

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 19", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    InitShader();
    InitBuffer();

    glfwSetKeyCallback(window, key_callback);

    float lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        drawScene(window, deltaTime);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}