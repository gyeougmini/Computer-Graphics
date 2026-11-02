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
#define M_PI 3.14159265358979323846f
#endif

GLuint shaderProgramID;
GLuint cubeVAO, cubeVBO[2];   // 0: 위치, 1: 색상
GLuint sphereVAO, sphereVBO;
int sphereIndexCount = 0;

// --- 공간 (정육면체/방) 크기 정의 ---
const float ROOM_HALF_X = 4.0f;
const float ROOM_HALF_Y = 4.0f;
const float ROOM_HALF_Z = 4.0f;

// --- 공간(정육면체) 자체의 변환 변수 ---
float roomTransZ = 0.0f;      // z/Z 키: 공간 Z축 이동
float roomRotY = 0.0f;        // y/Y 키: 공간 Y축 회전

// --- 바닥 열기 및 객체 낙하 변수 ---
float floorOpenAngle = 0.0f;  // 마우스 드래그로 조절되는 바닥 열림 각도 (0 ~ 90도)
bool isMousePressed = false;
double lastMouseX = 0.0;

// --- 공(Sphere) 구조체 ---
struct Ball {
    glm::vec3 pos;
    glm::vec3 vel;
    float radius;
    glm::vec3 color;
};
std::vector<Ball> balls;

// --- 바닥 육면체 장애물 구조체 ---
struct FloorCube {
    glm::vec3 pos;
    glm::vec3 vel;
    glm::vec3 scale;
    glm::vec3 color;
    bool isFalling;
};
std::vector<FloorCube> floorCubes;

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

void InitSphereBuffer(float radius, int slices, int stacks) {
    std::vector<float> vertices;
    for (int i = 0; i <= stacks; ++i) {
        float V = (float)i / (float)stacks;
        float phi = V * M_PI;

        for (int j = 0; j <= slices; ++j) {
            float U = (float)j / (float)slices;
            float theta = U * (M_PI * 2.0f);

            float x = cos(theta) * sin(phi);
            float y = cos(phi);
            float z = sin(theta) * sin(phi);

            vertices.push_back(x * radius);
            vertices.push_back(y * radius);
            vertices.push_back(z * radius);
        }
    }

    std::vector<unsigned int> indices;
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            unsigned int first = (i * (slices + 1)) + j;
            unsigned int second = first + slices + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    sphereIndexCount = (int)indices.size();

    GLuint ebo;
    glGenVertexArrays(1, &sphereVAO);
    glBindVertexArray(sphereVAO);

    glGenBuffers(1, &sphereVBO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void InitBuffer() {
    // 1. 육면체 버퍼
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

    // 2. 구 메쉬 초기화
    InitSphereBuffer(1.0f, 20, 20);

    // 3. 바닥 육면체 3개 생성 (Z축을 따라 일렬로 배치)
    floorCubes = {
        { glm::vec3(0.0f, -ROOM_HALF_Y + 0.3f, -2.5f), glm::vec3(0,0,0), glm::vec3(1.2f, 0.6f, 1.2f), glm::vec3(0.2f, 0.8f, 0.8f), false },
        { glm::vec3(0.0f, -ROOM_HALF_Y + 0.4f,  0.0f), glm::vec3(0,0,0), glm::vec3(1.8f, 0.8f, 1.8f), glm::vec3(0.8f, 0.8f, 0.2f), false },
        { glm::vec3(0.0f, -ROOM_HALF_Y + 0.5f,  2.5f), glm::vec3(0,0,0), glm::vec3(1.5f, 1.0f, 1.5f), glm::vec3(0.8f, 0.2f, 0.8f), false }
    };
}

void spawnBall() {
    if (balls.size() >= 5) return;

    Ball b;
    b.pos = glm::vec3(0.0f, 0.0f, 0.0f);
    float vx = ((rand() % 100) / 50.0f - 1.0f) * 4.0f;
    float vy = ((rand() % 100) / 50.0f - 1.0f) * 4.0f;
    float vz = ((rand() % 100) / 50.0f - 1.0f) * 4.0f;
    if (std::abs(vx) < 0.5f) vx = 2.0f;
    if (std::abs(vy) < 0.5f) vy = 2.0f;
    if (std::abs(vz) < 0.5f) vz = 2.0f;

    b.vel = glm::vec3(vx, vy, vz);
    b.radius = 0.5f;
    b.color = glm::vec3(0.2f, 0.3f, 0.9f); // 파란색 공
    balls.push_back(b);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    switch (key) {
    case GLFW_KEY_Q: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    case GLFW_KEY_B: spawnBall(); break;

        // z / Z: 정육면체(공간) Z축 이동
    case GLFW_KEY_Z:
        if (mods & GLFW_MOD_SHIFT) roomTransZ -= 0.3f; // Z (음의 방향)
        else roomTransZ += 0.3f;                      // z (양의 방향)
        break;

        // y / Y: 정육면체(공간) Y축 회전
    case GLFW_KEY_Y:
        if (mods & GLFW_MOD_SHIFT) roomRotY -= 3.0f;  // Y (반시계)
        else roomRotY += 3.0f;                         // y (시계)
        break;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            isMousePressed = true;
            glfwGetCursorPos(window, &lastMouseX, nullptr);
        }
        else if (action == GLFW_RELEASE) {
            isMousePressed = false;
        }
    }
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    if (isMousePressed) {
        float xoffset = (float)(xpos - lastMouseX);
        lastMouseX = xpos;

        // 마우스 드래그로 바닥 열림 각도 조절 (0 ~ 85도)
        floorOpenAngle += xoffset * 0.5f;
        floorOpenAngle = glm::clamp(floorOpenAngle, 0.0f, 85.0f);
    }
}

void update(float deltaTime) {
    // 1. 공 이동 및 충돌
    for (auto& b : balls) {
        b.pos += b.vel * deltaTime;

        if (b.pos.x - b.radius <= -ROOM_HALF_X || b.pos.x + b.radius >= ROOM_HALF_X) {
            b.vel.x = -b.vel.x;
            b.pos.x = glm::clamp(b.pos.x, -ROOM_HALF_X + b.radius, ROOM_HALF_X - b.radius);
        }
        // 바닥이 열려있는 동안은 바닥 경계 체크에서 아래로 빠질 수 있음
        if (b.pos.y + b.radius >= ROOM_HALF_Y) {
            b.vel.y = -b.vel.y;
            b.pos.y = ROOM_HALF_Y - b.radius;
        }
        if (b.pos.y - b.radius <= -ROOM_HALF_Y && floorOpenAngle < 20.0f) {
            b.vel.y = -b.vel.y;
            b.pos.y = -ROOM_HALF_Y + b.radius;
        }
        if (b.pos.z - b.radius <= -ROOM_HALF_Z || b.pos.z + b.radius >= ROOM_HALF_Z) {
            b.vel.z = -b.vel.z;
            b.pos.z = glm::clamp(b.pos.z, -ROOM_HALF_Z + b.radius, ROOM_HALF_Z - b.radius);
        }
    }

    // 2. 바닥이 일정 이상 열리면 육면체가 아래로 떨어짐 (바닥 열리기/객체 떨어지기)
    for (auto& fc : floorCubes) {
        if (floorOpenAngle > 25.0f) {
            fc.isFalling = true;
        }
        if (fc.isFalling) {
            fc.vel.y -= 9.8f * deltaTime; // 중력 가속도
            fc.pos += fc.vel * deltaTime;
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

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[1]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, colorBuffer.size() * sizeof(float), colorBuffer.data());

    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawSphere(const glm::mat4& model, const glm::mat4& viewProj, glm::vec3 color, GLuint transformLoc) {
    glm::mat4 mvp = viewProj * model;
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    std::vector<float> colorBuffer(sphereIndexCount * 3);
    for (int i = 0; i < sphereIndexCount; ++i) {
        colorBuffer[i * 3 + 0] = color.r;
        colorBuffer[i * 3 + 1] = color.g;
        colorBuffer[i * 3 + 2] = color.b;
    }

    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[1]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, colorBuffer.size() * sizeof(float), colorBuffer.data());

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLES, sphereIndexCount, GL_UNSIGNED_INT, 0);
}

// 공간 (왼쪽, 오른쪽, 아래쪽, 위쪽, 뒤쪽 면)
void drawRoom(const glm::mat4& roomModel, const glm::mat4& viewProj, GLuint transformLoc) {
    // 1. 뒤쪽 면 (Z = -ROOM_HALF_Z)
    glm::mat4 back = glm::translate(roomModel, glm::vec3(0.0f, 0.0f, -ROOM_HALF_Z));
    back = glm::scale(back, glm::vec3(ROOM_HALF_X * 2.0f, ROOM_HALF_Y * 2.0f, 0.1f));
    drawCube(back, viewProj, glm::vec3(0.7f, 0.7f, 0.7f), transformLoc);

    // 2. 왼쪽 면 (X = -ROOM_HALF_X)
    glm::mat4 left = glm::translate(roomModel, glm::vec3(-ROOM_HALF_X, 0.0f, 0.0f));
    left = glm::scale(left, glm::vec3(0.1f, ROOM_HALF_Y * 2.0f, ROOM_HALF_Z * 2.0f));
    drawCube(left, viewProj, glm::vec3(0.4f, 0.4f, 0.4f), transformLoc);

    // 3. 오른쪽 면 (X = ROOM_HALF_X)
    glm::mat4 right = glm::translate(roomModel, glm::vec3(ROOM_HALF_X, 0.0f, 0.0f));
    right = glm::scale(right, glm::vec3(0.1f, ROOM_HALF_Y * 2.0f, ROOM_HALF_Z * 2.0f));
    drawCube(right, viewProj, glm::vec3(0.5f, 0.5f, 0.5f), transformLoc);

    // 4. 위쪽 면 (Y = ROOM_HALF_Y)
    glm::mat4 top = glm::translate(roomModel, glm::vec3(0.0f, ROOM_HALF_Y, 0.0f));
    top = glm::scale(top, glm::vec3(ROOM_HALF_X * 2.0f, 0.1f, ROOM_HALF_Z * 2.0f));
    drawCube(top, viewProj, glm::vec3(0.3f, 0.3f, 0.3f), transformLoc);

    // 5. 아래쪽 면 (좌/우 2개의 문으로 split 되어 Z축 중심으로 회전하며 열림)
    // 5-1. 왼쪽 바닥문 (회전축: X = -ROOM_HALF_X)
    glm::mat4 leftFloor = glm::translate(roomModel, glm::vec3(-ROOM_HALF_X, -ROOM_HALF_Y, 0.0f));
    leftFloor = glm::rotate(leftFloor, glm::radians(-floorOpenAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    leftFloor = glm::translate(leftFloor, glm::vec3(ROOM_HALF_X * 0.5f, 0.0f, 0.0f));
    leftFloor = glm::scale(leftFloor, glm::vec3(ROOM_HALF_X, 0.1f, ROOM_HALF_Z * 2.0f));
    drawCube(leftFloor, viewProj, glm::vec3(0.6f, 0.6f, 0.6f), transformLoc);

    // 5-2. 오른쪽 바닥문 (회전축: X = ROOM_HALF_X)
    glm::mat4 rightFloor = glm::translate(roomModel, glm::vec3(ROOM_HALF_X, -ROOM_HALF_Y, 0.0f));
    rightFloor = glm::rotate(rightFloor, glm::radians(floorOpenAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    rightFloor = glm::translate(rightFloor, glm::vec3(-ROOM_HALF_X * 0.5f, 0.0f, 0.0f));
    rightFloor = glm::scale(rightFloor, glm::vec3(ROOM_HALF_X, 0.1f, ROOM_HALF_Z * 2.0f));
    drawCube(rightFloor, viewProj, glm::vec3(0.65f, 0.65f, 0.65f), transformLoc);
}

void drawScene(GLFWwindow* window, float deltaTime) {
    update(deltaTime);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);
    GLuint transformLoc = glGetUniformLocation(shaderProgramID, "transform");

    // 고정 카메라 설정 (원근 투영)
    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 12.0f);
    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 viewProj = projection * view;

    // --- 정육면체(공간) 자체 변환 행렬 ---
    glm::mat4 roomModel = glm::mat4(1.0f);
    roomModel = glm::translate(roomModel, glm::vec3(0.0f, 0.0f, roomTransZ)); // z / Z 키 이동
    roomModel = glm::rotate(roomModel, glm::radians(roomRotY), glm::vec3(0.0f, 1.0f, 0.0f)); // y / Y 키 회전

    // 1. 방/공간 렌더링
    drawRoom(roomModel, viewProj, transformLoc);

    // 2. 바닥 육면체 3개 렌더링 (바닥이 열리면 낙하)
    for (const auto& fc : floorCubes) {
        glm::mat4 model = glm::translate(roomModel, fc.pos);
        model = glm::scale(model, fc.scale);
        drawCube(model, viewProj, fc.color, transformLoc);
    }

    // 3. 공(Sphere) 렌더링
    for (const auto& b : balls) {
        glm::mat4 model = glm::translate(roomModel, b.pos);
        model = glm::scale(model, glm::vec3(b.radius));
        drawSphere(model, viewProj, b.color, transformLoc);
    }
}

int main() {
    srand((unsigned int)time(NULL));

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 21 - 3D Ball Bounce", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    InitShader();
    InitBuffer();

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_position_callback);

    // 초기 시작 공 1개 생성
    spawnBall();

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