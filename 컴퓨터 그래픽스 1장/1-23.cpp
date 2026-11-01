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

// --- 무대 문 상태 변수 (시작 시 내부가 바로 보이도록 1.0f로 설정) ---
bool doorOpen = true;        // o/O키로 열고 닫기
float doorProgress = 1.0f;   // 1.0 (열림) ~ 0.0 (닫힘)

// --- 로봇 상태 변수 ---
glm::vec3 robotPos = glm::vec3(0.0f, -1.25f, 0.0f);
float robotRotY = 180.0f; // 바라보는 방향
float moveSpeed = 1.5f;   // 걷기 속도
float limbAngle = 0.0f;   // 팔/다리 흔들림 각도
float limbDir = 1.0f;

// 점프 변수
bool isJumping = false;
float jumpVel = 0.0f;
float currentY = -1.25f; // 기본 바닥 Y 높이

// 장애물 3개 구조체 및 위치 데이터
struct Obstacle {
    glm::vec3 pos;
    glm::vec3 size;
    glm::vec3 color;
};
Obstacle obstacles[3];

// --- 카메라 조작 변수 (내부 입체 시야 확보를 위한 초기 시점) ---
glm::vec3 cameraPos = glm::vec3(0.0f, 1.5f, 5.5f);
bool animCamY = false;
float camYDir = 1.0f;
float camYAngle = 0.0f;

// 단위 육면체 버텍스 데이터 (-0.5 ~ 0.5)
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
}

// 장애물 초기화 (로봇 시작 위치와 겹침 방지 로직 포함)
void initObstacles() {
    for (int i = 0; i < 3; ++i) {
        float ox, oz;
        float safeDistance = 0.8f; // 로봇과의 최소 안전거리

        // 로봇 시작점 (0,0)과 겹치지 않는 좌표 추출
        do {
            ox = ((rand() % 100) / 50.0f - 1.0f) * 1.2f;
            oz = ((rand() % 100) / 50.0f - 1.0f) * 1.2f;
        } while (sqrt(ox * ox + oz * oz) < safeDistance);

        float sy = 0.3f + (rand() % 3) * 0.15f; // 장애물 높이

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

// AABB 충돌 체크 함수
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
    case GLFW_KEY_O: doorOpen = !doorOpen; break; // o/O: 문 슬라이딩 오픈[cite: 46]
    case GLFW_KEY_I: resetAll(); break;          // i: 리셋[cite: 46]

        // w/a/s/d: 이동 방향 변경[cite: 46]
    case GLFW_KEY_W: robotRotY = 180.0f; break; // 화면 안쪽 (-Z)
    case GLFW_KEY_S: robotRotY = 0.0f;   break; // 화면 앞쪽 (+Z)
    case GLFW_KEY_A: robotRotY = 270.0f; break; // 왼쪽 (-X)
    case GLFW_KEY_D: robotRotY = 90.0f;  break; // 오른쪽 (+X)

        // +/-: 속도 조절[cite: 46]
    case GLFW_KEY_EQUAL: moveSpeed = std::min(4.0f, moveSpeed + 0.5f); break;
    case GLFW_KEY_MINUS: moveSpeed = std::max(0.5f, moveSpeed - 0.5f); break;

        // j: 점프[cite: 46]
    case GLFW_KEY_J:
        if (!isJumping) {
            isJumping = true;
            jumpVel = 4.0f;
        }
        break;

        // 카메라 조작 (z/Z, x/X, y/Y)[cite: 46]
    case GLFW_KEY_Z: cameraPos.z += isShift ? -0.2f : 0.2f; break;
    case GLFW_KEY_X: cameraPos.x += isShift ? -0.2f : 0.2f; break;
    case GLFW_KEY_Y:
        animCamY = !animCamY;
        camYDir = isShift ? -1.0f : 1.0f;
        break;
    }
}

void update(float deltaTime) {
    // 1. 문 슬라이딩 애니메이션[cite: 46]
    if (doorOpen && doorProgress < 1.0f) {
        doorProgress += deltaTime * 1.5f;
        if (doorProgress > 1.0f) doorProgress = 1.0f;
    }
    else if (!doorOpen && doorProgress > 0.0f) {
        doorProgress -= deltaTime * 1.5f;
        if (doorProgress < 0.0f) doorProgress = 0.0f;
    }

    // 2. 카메라 Y축 공전[cite: 46]
    if (animCamY) {
        camYAngle += camYDir * 45.0f * deltaTime;
    }

    // 3. 로봇 자동 직진 및 충돌 체크[cite: 46]
    float rad = glm::radians(robotRotY);
    glm::vec3 dir = glm::vec3(sin(rad), 0.0f, cos(rad));
    glm::vec3 nextPos = robotPos + dir * moveSpeed * deltaTime;

    // 벽면 경계 충돌 (무대 벽 -1.7 ~ 1.7)[cite: 46]
    bool hitWall = (nextPos.x < -1.7f || nextPos.x > 1.7f || nextPos.z < -1.7f || nextPos.z > 1.7f);

    int hitObsIdx = -1;
    bool hitObs = checkObstacleCollision(nextPos, hitObsIdx);

    // 장애물 탑 높이 계산
    float targetHeight = -1.25f; // 기본 바닥 높이
    if (hitObsIdx != -1) {
        targetHeight = -2.0f + obstacles[hitObsIdx].size.y + 0.75f;
    }

    if (hitWall) {
        robotRotY = fmod(robotRotY + 180.0f, 360.0f); // 벽 부딪힘 시 반대 방향 회전[cite: 46]
    }
    else if (hitObs) {
        // 점프하여 장애물 위 높이보다 충분히 높으면 장애물 위로 이동 허용[cite: 46]
        if (robotPos.y >= targetHeight - 0.1f) {
            robotPos.x = nextPos.x;
            robotPos.z = nextPos.z;
            currentY = targetHeight;
        }
        else {
            robotRotY = fmod(robotRotY + 180.0f, 360.0f); // 아래에서 걸리면 회전[cite: 46]
        }
    }
    else {
        robotPos.x = nextPos.x;
        robotPos.z = nextPos.z;
        currentY = -1.25f; // 장애물에서 내려옴
    }

    // 4. 점프 및 물리 연산[cite: 46]
    if (isJumping) {
        robotPos.y += jumpVel * deltaTime;
        jumpVel -= 9.8f * deltaTime; // 중력 적용

        if (robotPos.y <= currentY) {
            robotPos.y = currentY;
            isJumping = false;
            jumpVel = 0.0f;
        }
    }
    else {
        // 부드러운 착지 높이 보정
        robotPos.y += (currentY - robotPos.y) * 10.0f * deltaTime;
    }

    // 5. 팔/다리 흔들림 애니메이션 (속도에 비례한 회전 각도)[cite: 46]
    float maxSwing = 15.0f * moveSpeed; // 속도가 빠르면 각도가 늘어남[cite: 46]
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

// 계층 구조 로봇 렌더링 함수[cite: 46]
void drawRobot(const glm::mat4& viewProj, GLuint transformLoc) {
    glm::mat4 robotRoot = glm::mat4(1.0f);
    robotRoot = glm::translate(robotRoot, robotPos);
    robotRoot = glm::rotate(robotRoot, glm::radians(robotRotY), glm::vec3(0.0f, 1.0f, 0.0f));

    // 색상 정의 (구분을 위한 전후/좌우 다른 색상)[cite: 46]
    glm::vec3 cBody = glm::vec3(0.2f, 0.4f, 0.8f); // 몸통 (파랑)
    glm::vec3 cHead = glm::vec3(0.9f, 0.8f, 0.6f); // 머리
    glm::vec3 cNose = glm::vec3(0.9f, 0.2f, 0.2f); // 코 (빨강, 앞면 구분)[cite: 46]
    glm::vec3 cArmLeft = glm::vec3(0.9f, 0.3f, 0.3f); // 왼팔 (빨강)[cite: 46]
    glm::vec3 cArmRight = glm::vec3(0.3f, 0.8f, 0.3f); // 오른팔 (초록)[cite: 46]
    glm::vec3 cLegLeft = glm::vec3(0.8f, 0.2f, 0.8f); // 왼다리 (보라)[cite: 46]
    glm::vec3 cLegRight = glm::vec3(0.8f, 0.8f, 0.2f); // 오른다리 (노랑)[cite: 46]

    // 1. 몸통 (Torso)[cite: 46]
    glm::mat4 torsoModel = glm::scale(robotRoot, glm::vec3(0.5f, 0.7f, 0.3f));
    drawCube(torsoModel, viewProj, cBody, transformLoc);

    // 2. 머리 (Head) & 코 (Nose)[cite: 46]
    glm::mat4 headModel = glm::translate(robotRoot, glm::vec3(0.0f, 0.5f, 0.0f));
    glm::mat4 headDraw = glm::scale(headModel, glm::vec3(0.35f, 0.35f, 0.35f));
    drawCube(headDraw, viewProj, cHead, transformLoc);

    glm::mat4 noseModel = glm::translate(headModel, glm::vec3(0.0f, 0.0f, 0.2f));
    noseModel = glm::scale(noseModel, glm::vec3(0.08f, 0.08f, 0.15f));
    drawCube(noseModel, viewProj, cNose, transformLoc); // 앞면 구분용 코[cite: 46]

    // 3. 양팔 (Left & Right Arms) - 서로 반대 각도 흔들림[cite: 46]
    // 왼팔
    glm::mat4 leftArmModel = glm::translate(robotRoot, glm::vec3(-0.33f, 0.25f, 0.0f));
    leftArmModel = glm::rotate(leftArmModel, glm::radians(limbAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    leftArmModel = glm::translate(leftArmModel, glm::vec3(0.0f, -0.25f, 0.0f));
    glm::mat4 leftArmDraw = glm::scale(leftArmModel, glm::vec3(0.12f, 0.5f, 0.12f));
    drawCube(leftArmDraw, viewProj, cArmLeft, transformLoc);

    // 오른팔
    glm::mat4 rightArmModel = glm::translate(robotRoot, glm::vec3(0.33f, 0.25f, 0.0f));
    rightArmModel = glm::rotate(rightArmModel, glm::radians(-limbAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    rightArmModel = glm::translate(rightArmModel, glm::vec3(0.0f, -0.25f, 0.0f));
    glm::mat4 rightArmDraw = glm::scale(rightArmModel, glm::vec3(0.12f, 0.5f, 0.12f));
    drawCube(rightArmDraw, viewProj, cArmRight, transformLoc);

    // 4. 양다리 (Left & Right Legs) - 서로 반대 각도 흔들림[cite: 46]
    // 왼다리
    glm::mat4 leftLegModel = glm::translate(robotRoot, glm::vec3(-0.15f, -0.35f, 0.0f));
    leftLegModel = glm::rotate(leftLegModel, glm::radians(-limbAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    leftLegModel = glm::translate(leftLegModel, glm::vec3(0.0f, -0.25f, 0.0f));
    glm::mat4 leftLegDraw = glm::scale(leftLegModel, glm::vec3(0.15f, 0.5f, 0.15f));
    drawCube(leftLegDraw, viewProj, cLegLeft, transformLoc);

    // 오른다리
    glm::mat4 rightLegModel = glm::translate(robotRoot, glm::vec3(0.15f, -0.35f, 0.0f));
    rightLegModel = glm::rotate(rightLegModel, glm::radians(limbAngle), glm::vec3(1.0f, 0.0f, 0.0f));
    rightLegModel = glm::translate(rightLegModel, glm::vec3(0.0f, -0.25f, 0.0f));
    glm::mat4 rightLegDraw = glm::scale(rightLegModel, glm::vec3(0.15f, 0.5f, 0.15f));
    drawCube(rightLegDraw, viewProj, cLegRight, transformLoc);
}

void drawScene(GLFWwindow* window, float deltaTime) {
    update(deltaTime);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE); // 내부가 항상 보이도록 면 컬링 활성화[cite: 46]
    glCullFace(GL_BACK);

    glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);
    GLuint transformLoc = glGetUniformLocation(shaderProgramID, "transform");

    // 카메라 뷰/투영 계산
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

    // --- 1. 5개 면 무대 벽면 그리기 (컬링 적용으로 카메라 회전 시에도 내부가 보임) ---[cite: 46]
    glm::vec3 cFloorStage = glm::vec3(0.4f, 0.6f, 0.4f); // 바닥 (초록)
    glm::vec3 cWallLeft = glm::vec3(0.2f, 0.5f, 0.3f); // 좌
    glm::vec3 cWallRight = glm::vec3(0.2f, 0.3f, 0.6f); // 우
    glm::vec3 cWallTop = glm::vec3(0.3f, 0.3f, 0.3f); // 상
    glm::vec3 cWallBack = glm::vec3(0.7f, 0.2f, 0.2f); // 후면

    // 바닥
    glm::mat4 mFloor = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f)), glm::vec3(4.0f, 0.05f, 4.0f));
    drawCube(mFloor, viewProj, cFloorStage, transformLoc);

    // 천장
    glm::mat4 mTop = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f)), glm::vec3(4.0f, 0.05f, 4.0f));
    drawCube(mTop, viewProj, cWallTop, transformLoc);

    // 좌측 벽
    glm::mat4 mLeft = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f)), glm::vec3(0.05f, 4.0f, 4.0f));
    drawCube(mLeft, viewProj, cWallLeft, transformLoc);

    // 우측 벽
    glm::mat4 mRight = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f)), glm::vec3(0.05f, 4.0f, 4.0f));
    drawCube(mRight, viewProj, cWallRight, transformLoc);

    // 후면 벽
    glm::mat4 mBack = glm::scale(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f)), glm::vec3(4.0f, 4.0f, 0.05f));
    drawCube(mBack, viewProj, cWallBack, transformLoc);

    // --- 2. 슬라이딩 전면 문 (O키로 위로 열림) ---[cite: 46]
    if (doorProgress < 1.0f) {
        float doorScaleY = 4.0f * (1.0f - doorProgress);
        float doorPosY = 2.0f - doorScaleY * 0.5f;

        glm::mat4 mDoor = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, doorPosY, 2.0f));
        mDoor = glm::scale(mDoor, glm::vec3(4.0f, doorScaleY, 0.05f));
        drawCube(mDoor, viewProj, glm::vec3(0.8f, 0.8f, 0.2f), transformLoc);
    }

    // --- 3. 랜덤 장애물 3개 렌더링 ---[cite: 46]
    for (int i = 0; i < 3; ++i) {
        glm::mat4 mObs = glm::translate(glm::mat4(1.0f), obstacles[i].pos);
        mObs = glm::scale(mObs, obstacles[i].size);
        drawCube(mObs, viewProj, obstacles[i].color, transformLoc);
    }

    // --- 4. 걷는 로봇 렌더링 ---[cite: 46]
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

    initObstacles(); // 겹침 없는 안전한 장애물 무작위 배치

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