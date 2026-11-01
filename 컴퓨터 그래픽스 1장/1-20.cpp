#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint shaderProgramID;
GLuint cubeVAO, cubeVBO[2]; // 0: 위치, 1: 색상

// --- 카메라 조작 변수 ---
bool isPerspective = true;        // p/P: 원근/직각 투영
float cameraAngleY = 45.0f;       // 좌/우 방향키: 카메라 Y축 회전 각도
float cameraRadius = 15.0f;       // +/-: 줌 인 / 줌 아웃 거리

// --- 컨베이어 벨트 및 상자 상태 변수 ---
bool isMoving = true;             // b: 벨트 이동 토글
float beltOffset = 0.0f;          // 벨트 줄무늬 이동 오프셋
float boxPositions[5] = { 0.0f, 2.5f, 5.0f, 7.5f, 10.0f }; // 상자 5개의 레일 상의 거리 위치

// 벨트 구간 및 부드러운 코너 궤적 설정
const float SEG1_LEN = 6.0f;                            // Z축 직진 구간 길이
const float CORNER_RADIUS = 0.5f;                        // 코너 원호 반지름
const float CORNER_LEN = 3.14159265f * CORNER_RADIUS / 2.0f; // 90도 원호 호 길이 (~0.785f)
const float SEG2_LEN = 6.0f;                            // X축 직진 구간 길이
const float TOTAL_LEN = SEG1_LEN + CORNER_LEN + SEG2_LEN; // 전체 트랙 길이

// 코너의 기하학적 중심 기준점 (원호 회전 중심)
const float CORNER_CENTER_X = 0.5f;
const float CORNER_CENTER_Z = 0.5f;

// 5개 상자 색상 정의
glm::vec3 boxColors[5] = {
    glm::vec3(0.8f, 0.2f, 0.2f), // 1번: 빨강
    glm::vec3(0.2f, 0.4f, 0.8f), // 2번: 파랑
    glm::vec3(0.2f, 0.7f, 0.3f), // 3번: 초록
    glm::vec3(0.9f, 0.6f, 0.1f), // 4번: 주황
    glm::vec3(0.8f, 0.8f, 0.2f)  // 5번: 노랑
};

// 단위 육면체 버텍스 위치 데이터 (-0.5 ~ 0.5)
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

    // Attribute 0: 정점 위치
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // Attribute 1: 정점 색상
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, 36 * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(1);
}

void resetAll() {
    isPerspective = true;
    cameraAngleY = 45.0f;
    cameraRadius = 15.0f;
    isMoving = true;
    beltOffset = 0.0f;
    for (int i = 0; i < 5; ++i) {
        boxPositions[i] = i * 2.5f;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    switch (key) {
    case GLFW_KEY_Q: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    case GLFW_KEY_B: isMoving = !isMoving; break;
    case GLFW_KEY_P: isPerspective = !isPerspective; break;
    case GLFW_KEY_R: resetAll(); break;

    case GLFW_KEY_EQUAL:
        cameraRadius -= 0.5f;
        if (cameraRadius < 3.0f) cameraRadius = 3.0f;
        break;
    case GLFW_KEY_MINUS:
        cameraRadius += 0.5f;
        if (cameraRadius > 30.0f) cameraRadius = 30.0f;
        break;

    case GLFW_KEY_LEFT:  cameraAngleY -= 3.0f; break;
    case GLFW_KEY_RIGHT: cameraAngleY += 3.0f; break;
    }
}

void update(float deltaTime) {
    if (!isMoving) return;

    float speed = 2.0f * deltaTime;

    // 벨트 오프셋 업데이트
    beltOffset += speed;
    if (beltOffset >= 0.4f) beltOffset -= 0.4f;

    // 상자 위치 및 무한 순환
    for (int i = 0; i < 5; ++i) {
        boxPositions[i] += speed;
        if (boxPositions[i] >= TOTAL_LEN) {
            boxPositions[i] -= TOTAL_LEN;
        }
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

// 상자 표면 숫자 점 표시
void drawBoxDots(int numDots, const glm::mat4& boxModel, const glm::mat4& viewProj, GLuint transformLoc) {
    glm::vec3 dotColor = glm::vec3(0.1f, 0.1f, 0.1f);

    float offsets[15][2] = {
        { 0.0f,  0.0f},
        {-0.2f, -0.2f}, { 0.2f,  0.2f},
        {-0.2f, -0.2f}, { 0.0f,  0.0f}, { 0.2f,  0.2f},
        {-0.2f, -0.2f}, { 0.2f, -0.2f}, {-0.2f,  0.2f}, { 0.2f,  0.2f},
        {-0.2f, -0.2f}, { 0.2f, -0.2f}, { 0.0f,  0.0f}, {-0.2f,  0.2f}, { 0.2f,  0.2f}
    };

    int count = numDots;
    int startIndex = 0;
    if (numDots == 2) startIndex = 1;
    else if (numDots == 3) startIndex = 3;
    else if (numDots == 4) startIndex = 6;
    else if (numDots == 5) startIndex = 10;

    for (int i = 0; i < count; ++i) {
        float dx = offsets[startIndex + i][0];
        float dz = offsets[startIndex + i][1];

        glm::mat4 dotModel = glm::translate(boxModel, glm::vec3(dx, 0.51f, dz));
        dotModel = glm::scale(dotModel, glm::vec3(0.12f, 0.02f, 0.12f));
        drawCube(dotModel, viewProj, dotColor, transformLoc);
    }
}

// 트랙 위치 거리(dist) 기반 위치 및 회전 행렬 계산 (원호 기반 부드러운 코너링)
glm::mat4 getTrackTransform(float dist, float heightOffset) {
    glm::mat4 model = glm::mat4(1.0f);

    if (dist <= SEG1_LEN) {
        // [구간 1] Z축 레일 (+Z -> 원점 방향)
        float posZ = (SEG1_LEN + CORNER_CENTER_Z) - dist;
        model = glm::translate(model, glm::vec3(0.0f, heightOffset, posZ));
    }
    else if (dist <= SEG1_LEN + CORNER_LEN) {
        // [코너 구간] 원호(Arc) 곡선 부드러운 이동 및 회전
        float t = (dist - SEG1_LEN) / CORNER_LEN; // 0.0 ~ 1.0
        float angleRad = t * (3.14159265f / 2.0f); // 0 -> PI/2 (90도)

        float posX = CORNER_CENTER_X - CORNER_RADIUS * cos(angleRad);
        float posZ = CORNER_CENTER_Z - CORNER_RADIUS * sin(angleRad);

        model = glm::translate(model, glm::vec3(posX, heightOffset, posZ));
        model = glm::rotate(model, -angleRad, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    else {
        // [구간 2] X축 레일 (원점 -> +X 방향)
        float posX = (dist - SEG1_LEN - CORNER_LEN) + CORNER_CENTER_X;
        model = glm::translate(model, glm::vec3(posX, heightOffset, 0.0f));
        model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    return model;
}

void drawScene(GLFWwindow* window, float deltaTime) {
    update(deltaTime);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.25f, 0.35f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);
    GLuint transformLoc = glGetUniformLocation(shaderProgramID, "transform");

    float radY = glm::radians(cameraAngleY);
    float camX = cameraRadius * sin(radY) * cos(glm::radians(30.0f));
    float camY = cameraRadius * sin(glm::radians(30.0f));
    float camZ = cameraRadius * cos(radY) * cos(glm::radians(30.0f));

    glm::vec3 cameraPos = glm::vec3(camX, camY, camZ);
    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(2.5f, 0.0f, 2.5f), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection;
    if (isPerspective) {
        projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    }
    else {
        float orthoSize = cameraRadius * 0.4f;
        projection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, 100.0f);
    }

    glm::mat4 viewProj = projection * view;

    glBindVertexArray(cubeVAO);

    glm::vec3 cGround = glm::vec3(0.3f, 0.35f, 0.3f);
    glm::vec3 cBeltFrame = glm::vec3(0.8f, 0.7f, 0.1f);
    glm::vec3 cBeltSurface = glm::vec3(0.2f, 0.2f, 0.2f);
    glm::vec3 cStripe = glm::vec3(0.4f, 0.4f, 0.4f);
    glm::vec3 cLeg = glm::vec3(0.15f, 0.15f, 0.15f);

    // 1. 바닥
    glm::mat4 groundModel = glm::mat4(1.0f);
    groundModel = glm::translate(groundModel, glm::vec3(2.5f, -0.5f, 2.5f));
    groundModel = glm::scale(groundModel, glm::vec3(20.0f, 0.1f, 20.0f));
    drawCube(groundModel, viewProj, cGround, transformLoc);

    // 2. 벨트 프레임 및 바닥 판 (코너 매끄럽게 완전 채움)
    // 2-1. 노란색 외곽 프레임 (직선 2개 + 코너 결합 판 1개)
    glm::mat4 seg1Frame = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.4f, 3.25f));
    seg1Frame = glm::scale(seg1Frame, glm::vec3(1.4f, 0.2f, 6.5f));
    drawCube(seg1Frame, viewProj, cBeltFrame, transformLoc);

    glm::mat4 seg2Frame = glm::translate(glm::mat4(1.0f), glm::vec3(3.25f, 0.4f, 0.0f));
    seg2Frame = glm::scale(seg2Frame, glm::vec3(6.5f, 0.2f, 1.4f));
    drawCube(seg2Frame, viewProj, cBeltFrame, transformLoc);

    glm::mat4 cornerFrame = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.4f, 0.0f));
    cornerFrame = glm::scale(cornerFrame, glm::vec3(1.4f, 0.201f, 1.4f));
    drawCube(cornerFrame, viewProj, cBeltFrame, transformLoc);

    // 2-2. 벨트 내부 어두운 면 (직선 2개 + 코너 결합 판 1개)
    glm::mat4 seg1Surf = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.51f, 3.25f));
    seg1Surf = glm::scale(seg1Surf, glm::vec3(1.0f, 0.05f, 6.5f));
    drawCube(seg1Surf, viewProj, cBeltSurface, transformLoc);

    glm::mat4 seg2Surf = glm::translate(glm::mat4(1.0f), glm::vec3(3.25f, 0.51f, 0.0f));
    seg2Surf = glm::scale(seg2Surf, glm::vec3(6.5f, 0.05f, 1.0f));
    drawCube(seg2Surf, viewProj, cBeltSurface, transformLoc);

    glm::mat4 cornerSurf = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.511f, 0.0f));
    cornerSurf = glm::scale(cornerSurf, glm::vec3(1.0f, 0.05f, 1.0f));
    drawCube(cornerSurf, viewProj, cBeltSurface, transformLoc);

    // 받침 다리 4개
    float legPositions[4] = { 1.5f, 5.5f, 1.5f, 5.5f };
    for (int i = 0; i < 2; ++i) {
        glm::mat4 leg = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, legPositions[i]));
        leg = glm::scale(leg, glm::vec3(0.15f, 0.8f, 0.15f));
        drawCube(leg, viewProj, cLeg, transformLoc);
    }
    for (int i = 2; i < 4; ++i) {
        glm::mat4 leg = glm::translate(glm::mat4(1.0f), glm::vec3(legPositions[i], 0.0f, 0.0f));
        leg = glm::scale(leg, glm::vec3(0.15f, 0.8f, 0.15f));
        drawCube(leg, viewProj, cLeg, transformLoc);
    }

    // 3. 연속적 줄무늬(Stripes) 렌더링
    for (float d = 0.0f; d <= TOTAL_LEN; d += 0.4f) {
        float currentDist = d + beltOffset;
        if (currentDist > TOTAL_LEN) currentDist -= TOTAL_LEN;

        glm::mat4 stripeModel = getTrackTransform(currentDist, 0.54f);

        if (currentDist > SEG1_LEN - 0.2f && currentDist < SEG1_LEN + CORNER_LEN + 0.2f) {
            stripeModel = glm::scale(stripeModel, glm::vec3(0.95f, 0.02f, 0.12f));
        }
        else {
            stripeModel = glm::scale(stripeModel, glm::vec3(0.95f, 0.02f, 0.08f));
        }
        drawCube(stripeModel, viewProj, cStripe, transformLoc);
    }

    // 4. 상자 5개 부드러운 코너링 렌더링
    for (int i = 0; i < 5; ++i) {
        float dist = boxPositions[i];
        glm::mat4 boxModel = getTrackTransform(dist, 0.95f);

        glm::mat4 drawBox = glm::scale(boxModel, glm::vec3(0.8f, 0.7f, 0.8f));
        drawCube(drawBox, viewProj, boxColors[i], transformLoc);

        drawBoxDots(i + 1, boxModel, viewProj, transformLoc);
    }
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 20 - Conveyor", nullptr, nullptr);
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