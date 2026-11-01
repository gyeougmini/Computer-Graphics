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

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

GLuint shaderProgramID;
GLuint cubeVAO, cubeVBO[2];
GLuint sphereVAO, sphereVBO[2], sphereEBO;
int sphereIndexCount = 0;

// 카메라 및 회전 조작
float spaceZ = 0.0f;
float spaceRotZ = 0.0f;
bool isDragging = false;
double lastMouseX = 0.0;
float boxRotZ = 0.0f;

// 공 구조체 및 상태
struct Ball {
    glm::vec3 pos;
    glm::vec3 vel;
    float radius;
    glm::vec3 color;
};
std::vector<Ball> balls;

// 룸 바운더리 크기 (원점 중심)
const float ROOM_SIZE = 4.0f; // -2.0 ~ +2.0

// 버텍스 / 셰이더 초기화
float cubeVertices[] = {
    -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f,
    -0.5f, -0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f, -0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,
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
}

void InitSphere(float radius, int rings, int sectors) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    float const R = 1.0f / (float)(rings - 1);
    float const S = 1.0f / (float)(sectors - 1);

    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            float y = sin(-M_PI / 2 + M_PI * r * R);
            float x = cos(2 * M_PI * s * S) * sin(M_PI * r * R);
            float z = sin(2 * M_PI * s * S) * sin(M_PI * r * R);

            vertices.push_back(x * radius);
            vertices.push_back(y * radius);
            vertices.push_back(z * radius);
        }
    }

    for (int r = 0; r < rings - 1; ++r) {
        for (int s = 0; s < sectors - 1; ++s) {
            indices.push_back(r * sectors + s);
            indices.push_back(r * sectors + (s + 1));
            indices.push_back((r + 1) * sectors + (s + 1));

            indices.push_back(r * sectors + s);
            indices.push_back((r + 1) * sectors + (s + 1));
            indices.push_back((r + 1) * sectors + s);
        }
    }

    sphereIndexCount = (int)indices.size();

    glGenVertexArrays(1, &sphereVAO);
    glBindVertexArray(sphereVAO);
    glGenBuffers(2, sphereVBO);
    glGenBuffers(1, &sphereEBO);

    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, sphereEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
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

    InitSphere(0.2f, 20, 20);
}

void addBall() {
    if (balls.size() >= 5) return; // 최대 5개 제한
    Ball b;
    b.pos = glm::vec3(0.0f, 0.0f, 0.0f);
    float vx = ((rand() % 100) / 50.0f - 1.0f) * 2.0f;
    float vy = ((rand() % 100) / 50.0f - 1.0f) * 2.0f;
    float vz = ((rand() % 100) / 50.0f - 1.0f) * 2.0f;
    b.vel = glm::vec3(vx, vy, vz);
    b.radius = 0.2f;
    b.color = glm::vec3(0.2f, 0.3f, 0.9f);
    balls.push_back(b);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    bool isShift = (mods & GLFW_MOD_SHIFT) != 0;

    switch (key) {
    case GLFW_KEY_Q: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    case GLFW_KEY_Z: spaceZ += isShift ? -0.2f : 0.2f; break; // z/Z
    case GLFW_KEY_Y: spaceRotZ += isShift ? -3.0f : 3.0f; break; // y/Y
    case GLFW_KEY_B: addBall(); break; // B키: 볼 추가
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isDragging = true;
            glfwGetCursorPos(window, &lastMouseX, NULL);
        }
        else if (action == GLFW_RELEASE) {
            isDragging = false;
        }
    }
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    if (isDragging) {
        double dx = xpos - lastMouseX;
        boxRotZ += (float)dx * 0.5f; // 마우스 좌우 드래그로 회전
        lastMouseX = xpos;
    }
}

void update(float deltaTime) {
    // 공 충돌 및 물리 이동
    for (auto& b : balls) {
        b.pos += b.vel * deltaTime;

        float limit = 1.8f; // 벽 충돌 범위
        if (b.pos.x < -limit) { b.pos.x = -limit; b.vel.x *= -1.0f; }
        if (b.pos.x > limit) { b.pos.x = limit; b.vel.x *= -1.0f; }
        if (b.pos.y < -limit) { b.pos.y = -limit; b.vel.y *= -1.0f; }
        if (b.pos.y > limit) { b.pos.y = limit; b.vel.y *= -1.0f; }
        if (b.pos.z < -limit) { b.pos.z = -limit; b.vel.z *= -1.0f; }
        if (b.pos.z > limit) { b.pos.z = limit; b.vel.z *= -1.0f; }
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

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawSphere(const glm::mat4& model, const glm::mat4& viewProj, glm::vec3 color, GLuint transformLoc) {
    glm::mat4 mvp = viewProj * model;
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
}

void drawScene(GLFWwindow* window, float deltaTime) {
    update(deltaTime);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);
    GLuint transformLoc = glGetUniformLocation(shaderProgramID, "transform");

    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 6.0f + spaceZ);
    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, spaceZ), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

    // 전체 공간 Z축 회전 적용
    view = glm::rotate(view, glm::radians(spaceRotZ), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 viewProj = projection * view;

    // 1. 5개의 방 벽면 그리기 (좌, 우, 상, 하, 후면)
    glm::vec3 wallColors[5] = {
        glm::vec3(0.4f, 0.4f, 0.4f), // 좌
        glm::vec3(0.45f, 0.45f, 0.45f), // 우
        glm::vec3(0.35f, 0.35f, 0.35f), // 상
        glm::vec3(0.6f, 0.6f, 0.6f), // 하 (바닥)
        glm::vec3(0.5f, 0.5f, 0.5f)  // 후면
    };

    // 좌측 벽
    glm::mat4 mLeft = glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f, 0.0f, 0.0f));
    mLeft = glm::scale(mLeft, glm::vec3(0.05f, 4.0f, 4.0f));
    drawCube(mLeft, viewProj, wallColors[0], transformLoc);

    // 우측 벽
    glm::mat4 mRight = glm::translate(glm::mat4(1.0f), glm::vec3(2.0f, 0.0f, 0.0f));
    mRight = glm::scale(mRight, glm::vec3(0.05f, 4.0f, 4.0f));
    drawCube(mRight, viewProj, wallColors[1], transformLoc);

    // 상단 벽
    glm::mat4 mTop = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f));
    mTop = glm::scale(mTop, glm::vec3(4.0f, 0.05f, 4.0f));
    drawCube(mTop, viewProj, wallColors[2], transformLoc);

    // 하단 벽 (바닥)
    glm::mat4 mBottom = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -2.0f, 0.0f));
    mBottom = glm::scale(mBottom, glm::vec3(4.0f, 0.05f, 4.0f));
    drawCube(mBottom, viewProj, wallColors[3], transformLoc);

    // 후면 벽
    glm::mat4 mBack = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.0f));
    mBack = glm::scale(mBack, glm::vec3(4.0f, 4.0f, 0.05f));
    drawCube(mBack, viewProj, wallColors[4], transformLoc);

    // 2. 바닥에 Z축을 따라 배치된 회전용 빨간 육면체들
    for (int i = 0; i < 3; ++i) {
        glm::mat4 mBox = glm::mat4(1.0f);
        mBox = glm::rotate(mBox, glm::radians(boxRotZ), glm::vec3(0.0f, 0.0f, 1.0f)); // 드래그 회전
        mBox = glm::translate(mBox, glm::vec3(0.0f, -1.7f + i * 0.15f, 1.0f - i * 1.0f));
        mBox = glm::scale(mBox, glm::vec3(0.4f + i * 0.1f, 0.4f + i * 0.1f, 0.4f + i * 0.1f));
        drawCube(mBox, viewProj, glm::vec3(0.9f, 0.2f, 0.2f), transformLoc);
    }

    // 3. 튀기는 공(Sphere)들 렌더링
    for (const auto& b : balls) {
        glm::mat4 mSphere = glm::translate(glm::mat4(1.0f), b.pos);
        drawSphere(mSphere, viewProj, b.color, transformLoc);
    }
}

int main() {
    srand((unsigned int)time(NULL));
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 21", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    glewInit();

    InitShader();
    InitBuffer();

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);

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