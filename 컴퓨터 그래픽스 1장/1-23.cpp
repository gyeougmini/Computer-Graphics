#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint shaderProgramID;
GLuint cubeVAO, cubeVBO[2];
GLuint stageVAO, stageVBO[2];

// --- 무대 문 상태 변수 ---
bool doorOpen = true;        // o/O키로 열고 닫기
float doorProgress = 1.0f;   // 1.0 (열림) ~ 0.0 (닫힘)

// --- 로봇 상태 변수 ---
glm::vec3 robotPos = glm::vec3(0.0f, -1.25f, 0.0f);
float robotRotY = 180.0f;
float moveSpeed = 1.5f;
float limbAngle = 0.0f;
float limbDir = 1.0f;

// 점프 변수
bool isJumping = false;
float jumpVel = 0.0f;
float currentY = -1.25f;

// 장애물 3개
struct Obstacle {
    glm::vec3 pos;
    glm::vec3 size;
    glm::vec3 color;
};
Obstacle obstacles[3];

// --- 카메라 조작 변수 ---
glm::vec3 cameraPos = glm::vec3(0.0f, 1.5f, 5.5f);
bool animCamY = false;
float camYDir = 1.0f;
float camYAngle = 0.0f;

// 일반 큐브 버텍스 데이터 (-0.5 ~ 0.5) - 로봇 및 장애물용 (겉면이 Front)
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

// 무대 내부 5개 면 버텍스 데이터 (안쪽이 Front가 되도록 CCW 정의)
float stageVertices[] = {
    // 0. 바닥 (Floor: Y = -2.0, 위쪽 +Y를 바라봄)
    -2.0f, -2.0f, -2.0f,   2.0f, -2.0f,  2.0f,   2.0f, -2.0f, -2.0f,
    -2.0f, -2.0f, -2.0f,  -2.0f, -2.0f,  2.0f,   2.0f, -2.0f,  2.0f,

    // 1. 천장 (Ceiling: Y = 2.0, 아래쪽 -Y를 바라봄)
    -2.0f,  2.0f, -2.0f,   2.0f,  2.0f, -2.0f,   2.0f,  2.0f,  2.0f,
    -2.0f,  2.0f, -2.0f,   2.0f,  2.0f,  2.0f,  -2.0f,  2.0f,  2.0f,

    // 2. 좌측 벽 (Left Wall: X = -2.0, 오른쪽 +X를 바라봄)
    -2.0f, -2.0f, -2.0f,  -2.0f,  2.0f, -2.0f,  -2.0f,  2.0f,  2.0f,
    -2.0f, -2.0f, -2.0f,  -2.0f,  2.0f,  2.0f,  -2.0f, -2.0f,  2.0f,

    // 3. 우측 벽 (Right Wall: X = 2.0, 왼쪽 -X를 바라봄)
     2.0f, -2.0f, -2.0f,   2.0f, -2.0f,  2.0f,   2.0f,  2.0f,  2.0f,
     2.0f, -2.0f, -2.0f,   2.0f,  2.0f,  2.0f,   2.0f,  2.0f, -2.0f,

     // 4. 후면 벽 (Back Wall: Z = -2.0, 앞쪽 +Z를 바라봄)
     -2.0f, -2.0f, -2.0f,   2.0f, -2.0f, -2.0f,   2.0f,  2.0f, -2.0f,
     -2.0f, -2.0f, -2.0f,   2.0f,  2.0f, -2.0f,  -2.0f,  2.0f, -2.0f
};

std::string filetobuf(const char* file) {
    std::ifstream f(file);
    if (!f.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
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
    // 1. 일반 큐브 (로봇, 장애물용)
    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);
    glGenBuffers(2, cubeVBO);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, 36 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);

    // 2. 무대 컬링 전용 VAO
    glGenVertexArrays(1, &stageVAO);
    glBindVertexArray(stageVAO);
    glGenBuffers(2, stageVBO);

    glBindBuffer(GL_ARRAY_BUFFER, stageVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(stageVertices), stageVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, stageVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, 30 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);
}

void initObstacles() {
    for (int i = 0; i < 3; ++i) {
        float ox, oz;
        float safeDistance = 0.8f;

        do {
            ox = ((rand() % 100) / 50.0f - 1.0f) * 1.2f;
            oz = ((rand() % 100) / 50.0f - 1.0f) * 1.2f;
        } while (sqrt(ox * ox + oz * oz) < safeDistance);

        float sy = 0.3f + (rand() % 3) * 0.15f;

        obstacles[i].size = glm::vec3(0.6f, sy, 0.6f);
        obstacles[i].pos = glm::vec3(ox, -2.0f + sy * 0.5f, oz);
        obstacles[i].color = glm::vec3(0.8f - i * 0.2f, 0.5f + i * 0.1f, 0.2f + i * 0.2f);
    }
}

void resetAll() {
    doorOpen = true;
    doorProgress = 1.0f;
    robotPos = glm::vec3(0.0f, -1.25f, 0.0f);
    robotRotY = 180.0f;
    moveSpeed = 1.5f;
    limbAngle = 0.0f;
    isJumping = false;
    jumpVel = 0.0f;
    currentY = -1.25f;

    cameraPos = glm::vec3(0.0f, 1.5f, 5.5f);
    animCamY = false;
    camYAngle = 0.0f;

    initObstacles();
}

bool checkObstacleCollision(glm::vec3 targetPos, int& hitIndex) {
    float robotRadius = 0.25f;
    hitIndex = -1;

    for (int i = 0; i < 3; ++i) {
        float minX = obstacles[i].pos.x - obstacles[i].size.x * 0.5f - robotRadius;
        float maxX = obstacles[i].pos.x + obstacles[i].size.x * 0.5f + robotRadius;
        float minZ = obstacles[i].pos.z - obstacles[i].size.z * 0.5f - robotRadius;
        float maxZ = obstacles[i].pos.z + obstacles[i].size.z * 0.5f + robotRadius;

        if (targetPos.x >= minX && targetPos.x <= maxX &&
            targetPos.z >= minZ && targetPos.z <= maxZ) {
            hitIndex = i;
            return true;
        }
    }
    return false;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    bool isShift = (mods & GLFW_MOD_SHIFT) != 0;

    switch (key) {
    case GLFW_KEY_Q: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    case GLFW_KEY_O: doorOpen = !doorOpen; break;
    case GLFW_KEY_I: resetAll(); break;

    case GLFW_KEY_W: robotRotY = 180.0f; break;
    case GLFW_KEY_S: robotRotY = 0.0f;   break;
    case GLFW_KEY_A: robotRotY = 270.0f; break;
    case GLFW_KEY_D: robotRotY = 90.0f;  break;

    case GLFW_KEY_EQUAL: moveSpeed = std::min(4.0f, moveSpeed + 0.5f); break;
    case GLFW_KEY_MINUS: moveSpeed = std::max(0.5f, moveSpeed - 0.5f); break;

    case GLFW_KEY_J:
        if (!isJumping) {
            isJumping = true;
            jumpVel = 4.0f;
        }
        break;

    case GLFW_KEY_Z: cameraPos.z += isShift ? -0.2f : 0.2f; break;
    case GLFW_KEY_X: cameraPos.x += isShift ? -0.2f : 0.2f; break;
    case GLFW_KEY_Y:
        animCamY = !animCamY;
        camYDir = isShift ? -1.0f : 1.0f;
        break;
    }
}

void update(float deltaTime) {
    if (doorOpen && doorProgress < 1.0f) {
        doorProgress += deltaTime * 1.5f;
        if (doorProgress > 1.0f) doorProgress = 1.0f;
    }
    else if (!doorOpen && doorProgress > 0.0f) {
        doorProgress -= deltaTime * 1.5f;
        if (doorProgress < 0.0f) doorProgress = 0.0f;
    }

    if (animCamY) {
        camYAngle += camYDir * 45.0f * deltaTime;
    }

    float rad = glm::radians(robotRotY);
    glm::vec3 dir = glm::vec3(sin(rad), 0.0f, cos(rad));
    glm::vec3 nextPos = robotPos + dir * moveSpeed * deltaTime;

    bool hitWall = (nextPos.x < -1.7f || nextPos.x > 1.7f || nextPos.z < -1.7f || nextPos.z > 1.7f);

    int hitObsIdx = -1;
    bool hitObs = checkObstacleCollision(nextPos, hitObsIdx);

    float targetHeight = -1.25f;
    if (hitObsIdx != -1) {
        targetHeight = -2.0f + obstacles[hitObsIdx].size.y + 0.75f;
    }

    if (hitWall) {
        robotRotY = fmod(robotRotY + 180.0f, 360.0f);
    }
    else if (hitObs) {
        if (robotPos.y >= targetHeight - 0.1f) {
            robotPos.x = nextPos.x;
            robotPos.z = nextPos.z;
            currentY = targetHeight;
        }
        else {
            robotRotY = fmod(robotRotY + 180.0f, 360.0f);
        }
    }
    else {
        robotPos.x = nextPos.x;
        robotPos.z = nextPos.z;
        currentY = -1.25f;
    }

    if (isJumping) {
        robotPos.y += jumpVel * deltaTime;
        jumpVel -= 9.8f * deltaTime;

        if (robotPos.y <= currentY) {
            robotPos.y = currentY;
            isJumping = false;
            jumpVel = 0.0f;
        }
    }
    else {
        robotPos.y += (currentY - robotPos.y) * 10.0f * deltaTime;
    }

    float maxSwing = 15.0f * moveSpeed;
    limbAngle += limbDir * moveSpeed * 120.0f * deltaTime;
    if (limbAngle > maxSwing) { limbAngle = maxSwing; limbDir = -1.0f; }
    if (limbAngle < -maxSwing) { limbAngle = -maxSwing; limbDir = 1.0f; }
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

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

// 무대 5개 면 개별 렌더링 함수
void drawStageWallFace(int faceIndex, const glm::mat4& viewProj, glm::vec3 color, GLuint transformLoc) {
    glm::mat4 mvp = viewProj * glm::mat4(1.0f);
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    std::vector<float> colorBuffer(6 * 3);
    for (int i = 0; i < 6; ++i) {
        colorBuffer[i * 3 + 0] = color.r;
        colorBuffer[i * 3 + 1] = color.g;
        colorBuffer[i * 3 + 2] = color.b;
    }

    glBindBuffer(GL_ARRAY_BUFFER, stageVBO[1]);
    glBufferSubData(GL_ARRAY_BUFFER, faceIndex * 6 * 3 * sizeof(float), colorBuffer.size() * sizeof(float), colorBuffer.data());

    glBindVertexArray(stageVAO);
    glDrawArrays(GL_TRIANGLES, faceIndex * 6, 6);
}

void drawRobot(const glm::mat4& viewProj, GLuint transformLoc) {
    glm::mat4 robotRoot = glm::mat4(1.0f);
    robotRoot = glm::translate(robotRoot, robotPos);
    robotRoot = glm::rotate(robotRoot, glm::radians(robotRotY), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 cBody = glm::vec3(0.2f, 0.4f, 0.8f);
    glm::vec3 cHead = glm::vec3(0.9f, 0.8f, 0.6f);
    glm::vec3 cNose = glm::vec3(0.9f, 0.2f, 0.2f);
    glm::vec3 cArmLeft = glm::vec3(0.9f, 0.3f, 0.3f);
    glm::vec3 cArmRight = glm::vec3(0.3f, 0.8f, 0.3f);
    glm::vec3 cLegLeft = glm::vec3(0.8f, 0.2f, 0.8f);
    glm::vec3 cLegRight = glm::vec3(0.8f, 0.8f, 0.2f);

    // 1. 몸통
    glm::mat4 torsoModel = glm::scale(robotRoot, glm::vec3(0.5f, 0.7f, 0.3f));
    drawCube(torsoModel, viewProj, cBody, transformLoc);

    // 2. 머리 & 코
    glm::mat4 headModel = glm::translate(robotRoot, glm::vec3(0.0f, 0.5f, 0.0f));
    glm::mat4 headDraw = glm::scale(headModel, glm::vec3(0.35f, 0.35f, 0.35f));
    drawCube(headDraw, viewProj, cHead, transformLoc);

    glm::mat4 noseModel = glm::translate(headModel, glm::vec3(0.0f, 0.0f, 0.2f));
    noseModel = glm::scale(noseModel, glm::vec3(0.08f, 0.08f, 0.15f));
    drawCube(noseModel, viewProj, cNose, transformLoc);

    // 3. 양팔
    glm::mat4 leftArmModel = glm::translate(robotRoot, glm::vec3(-0.33f, 0.25f, 0.0f));
    leftArmModel = glm::rotate(leftArmModel, glm::radians(limbAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    leftArmModel = glm::translate(leftArmModel, glm::vec3(0.0f, -0.25f, 0.0f));
    glm::mat4 leftArmDraw = glm::scale(leftArmModel, glm::vec3(0.12f, 0.5f, 0.12f));
    drawCube(leftArmDraw, viewProj, cArmLeft, transformLoc);

    glm::mat4 rightArmModel = glm::translate(robotRoot, glm::vec3(0.33f, 0.25f, 0.0f));
    rightArmModel = glm::rotate(rightArmModel, glm::radians(-limbAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    rightArmModel = glm::translate(rightArmModel, glm::vec3(0.0f, -0.25f, 0.0f));
    glm::mat4 rightArmDraw = glm::scale(rightArmModel, glm::vec3(0.12f, 0.5f, 0.12f));
    drawCube(rightArmDraw, viewProj, cArmRight, transformLoc);

    // 4. 양다리
    glm::mat4 leftLegModel = glm::translate(robotRoot, glm::vec3(-0.15f, -0.35f, 0.0f));
    leftLegModel = glm::rotate(leftLegModel, glm::radians(-limbAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    leftLegModel = glm::translate(leftLegModel, glm::vec3(0.0f, -0.25f, 0.0f));
    glm::mat4 leftLegDraw = glm::scale(leftLegModel, glm::vec3(0.15f, 0.5f, 0.15f));
    drawCube(leftLegDraw, viewProj, cLegLeft, transformLoc);

    glm::mat4 rightLegModel = glm::translate(robotRoot, glm::vec3(0.15f, -0.35f, 0.0f));
    rightLegModel = glm::rotate(rightLegModel, glm::radians(limbAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    rightLegModel = glm::translate(rightLegModel, glm::vec3(0.0f, -0.25f, 0.0f));
    glm::mat4 rightLegDraw = glm::scale(rightLegModel, glm::vec3(0.15f, 0.5f, 0.15f));
    drawCube(rightLegDraw, viewProj, cLegRight, transformLoc);
}

void drawScene(GLFWwindow* window, float deltaTime) {
    update(deltaTime);

    glEnable(GL_DEPTH_TEST);

    // --- 은면 제거 컬링 활성화 ---
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK); // 뒷면 자동 제거

    glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);
    GLuint transformLoc = glGetUniformLocation(shaderProgramID, "transform");

    glm::vec3 currentCamPos = cameraPos;
    if (animCamY) {
        float rad = glm::radians(camYAngle);
        float x = cameraPos.x * cos(rad) + cameraPos.z * sin(rad);
        float z = -cameraPos.x * sin(rad) + cameraPos.z * cos(rad);
        currentCamPos = glm::vec3(x, cameraPos.y, z);
    }

    glm::mat4 view = glm::lookAt(currentCamPos, glm::vec3(0.0f, -0.5f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 viewProj = projection * view;

    // --- 1. 무대 5개 면 (바닥, 천장, 좌, 우, 후면) 렌더링 ---
    glm::vec3 cFloorStage = glm::vec3(0.4f, 0.6f, 0.4f); // 바닥 (초록)
    glm::vec3 cWallTop = glm::vec3(0.3f, 0.3f, 0.3f); // 상단 (회색)
    glm::vec3 cWallLeft = glm::vec3(0.2f, 0.5f, 0.3f); // 좌측 (진녹색)
    glm::vec3 cWallRight = glm::vec3(0.2f, 0.3f, 0.6f); // 우측 (파랑색)
    glm::vec3 cWallBack = glm::vec3(0.7f, 0.2f, 0.2f); // 후면 (빨강색)

    drawStageWallFace(0, viewProj, cFloorStage, transformLoc); // 0: 바닥
    drawStageWallFace(1, viewProj, cWallTop, transformLoc); // 1: 천장
    drawStageWallFace(2, viewProj, cWallLeft, transformLoc); // 2: 좌측
    drawStageWallFace(3, viewProj, cWallRight, transformLoc); // 3: 우측
    drawStageWallFace(4, viewProj, cWallBack, transformLoc); // 4: 후면

    // --- 2. 슬라이딩 전면 문 (O키) ---
    if (doorProgress < 1.0f) {
        float doorScaleY = 4.0f * (1.0f - doorProgress);
        float doorPosY = 2.0f - doorScaleY * 0.5f;

        glm::mat4 mDoor = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, doorPosY, 2.0f));
        mDoor = glm::scale(mDoor, glm::vec3(4.0f, doorScaleY, 0.05f));
        drawCube(mDoor, viewProj, glm::vec3(0.8f, 0.8f, 0.2f), transformLoc);
    }

    // --- 3. 장애물 3개 ---
    for (int i = 0; i < 3; ++i) {
        glm::mat4 mObs = glm::translate(glm::mat4(1.0f), obstacles[i].pos);
        mObs = glm::scale(mObs, obstacles[i].size);
        drawCube(mObs, viewProj, obstacles[i].color, transformLoc);
    }

    // --- 4. 걷는 사람 로봇 ---
    drawRobot(viewProj, transformLoc);

    glDisable(GL_CULL_FACE);
}

int main() {
    srand((unsigned int)time(NULL));

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 23 - Robot Stage", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit();

    InitShader();
    InitBuffer();

    initObstacles();

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