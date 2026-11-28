// 실습 30: 육면체와 사각뿔에 텍스처 입히기
//  - 육면체 6면, 사각뿔 5면(옆면 4 + 바닥 1)에 각각 다른 텍스처
//  - 키: c 육면체 / p 사각뿔 / x,X x축 회전 / y,Y y축 회전 / s 초기화 / q 종료
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint shaderProgramID;

// uniform 위치
GLint modelLoc, viewLoc, projLoc, objectColorLoc;
GLint lightPosLoc, lightColorLoc, viewPosLoc, lightOnLoc;
GLint useTextureLoc, samplerLoc;

// 도형: 0 = 육면체, 1 = 사각뿔
GLuint VAO[3], VBO[3];          // 0: 육면체, 1: 사각뿔, 2: 좌표축
int cubeVertexCount = 0, pyramidVertexCount = 0;

// 텍스처 6장 (A~F)
const int TEX_COUNT = 6;
GLuint textures[TEX_COUNT];
const char* texFiles[TEX_COUNT] = { "A.png", "B.png", "C.png", "D.png", "E.png", "F.png" };

// --- 상태 변수 ---
int shapeMode = 0;                         // c: 0, p: 1
const float initRotX = 25.0f, initRotY = -35.0f;
float rotX = initRotX, rotY = initRotY;
bool animX = false; float xDir = 1.0f;     // x/X
bool animY = false; float yDir = 1.0f;     // y/Y

glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.5f);
glm::vec3 lightPos = glm::vec3(2.0f, 3.0f, 4.0f);

// ======================= 셰이더 =======================
std::string filetobuf(const char* file) {
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open()) {
        std::cerr << "파일 열기 실패: " << file << std::endl;
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
}

void CheckShader(GLuint shader, const char* name) {
    GLint ok;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, 1024, NULL, log);
        std::cerr << name << " 컴파일 에러:\n" << log << std::endl;
    }
}

void InitShader() {
    std::string vStr = filetobuf("vertex30.glsl");
    std::string fStr = filetobuf("fragment30.glsl");
    const char* vSrc = vStr.c_str();
    const char* fSrc = fStr.c_str();

    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vSrc, NULL);
    glCompileShader(vShader);
    CheckShader(vShader, "vertex26.glsl");

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fSrc, NULL);
    glCompileShader(fShader);
    CheckShader(fShader, "fragment26.glsl");

    shaderProgramID = glCreateProgram();
    glAttachShader(shaderProgramID, vShader);
    glAttachShader(shaderProgramID, fShader);
    glLinkProgram(shaderProgramID);

    glDeleteShader(vShader);
    glDeleteShader(fShader);

    modelLoc = glGetUniformLocation(shaderProgramID, "model");
    viewLoc = glGetUniformLocation(shaderProgramID, "view");
    projLoc = glGetUniformLocation(shaderProgramID, "projection");
    objectColorLoc = glGetUniformLocation(shaderProgramID, "objectColor");
    lightPosLoc = glGetUniformLocation(shaderProgramID, "lightPos");
    lightColorLoc = glGetUniformLocation(shaderProgramID, "lightColor");
    viewPosLoc = glGetUniformLocation(shaderProgramID, "viewPos");
    lightOnLoc = glGetUniformLocation(shaderProgramID, "lightOn");
    useTextureLoc = glGetUniformLocation(shaderProgramID, "useTexture");
    samplerLoc = glGetUniformLocation(shaderProgramID, "outTexture");

    glUseProgram(shaderProgramID);
    glUniform1i(samplerLoc, 0);   // 샘플러 outTexture → 텍스처 유닛 0
}

// ======================= 버텍스 데이터 =======================
// 정점 1개 = 위치(3) + 법선(3) + 텍스처 좌표(2) = 8 float, stride 32바이트
void pushVertex(std::vector<float>& v, glm::vec3 p, glm::vec3 n, glm::vec2 uv) {
    v.insert(v.end(), { p.x, p.y, p.z, n.x, n.y, n.z, uv.x, uv.y });
}

// 사각형 면 1개 (삼각형 2개 = 정점 6개)
//  center: 면 중심, n: 바깥쪽 법선, r: 텍스처의 오른쪽(+s) 방향, u: 텍스처의 위쪽(+t) 방향
//  r × u = n 이 되도록 잡으면 바깥에서 봤을 때 반시계(CCW) + 글자가 똑바로 보인다.
void pushQuad(std::vector<float>& v, glm::vec3 center, glm::vec3 n, glm::vec3 r, glm::vec3 u) {
    glm::vec3 bl = center - r * 0.5f - u * 0.5f;   // 왼쪽 아래 (0,0)
    glm::vec3 br = center + r * 0.5f - u * 0.5f;   // 오른쪽 아래 (1,0)
    glm::vec3 tr = center + r * 0.5f + u * 0.5f;   // 오른쪽 위 (1,1)
    glm::vec3 tl = center - r * 0.5f + u * 0.5f;   // 왼쪽 위 (0,1)
    pushVertex(v, bl, n, { 0, 0 }); pushVertex(v, br, n, { 1, 0 }); pushVertex(v, tr, n, { 1, 1 });
    pushVertex(v, tr, n, { 1, 1 }); pushVertex(v, tl, n, { 0, 1 }); pushVertex(v, bl, n, { 0, 0 });
}

// 삼각형 면 1개 (사각뿔 옆면): 밑변 왼쪽(0,0) → 밑변 오른쪽(1,0) → 꼭짓점(0.5,1)
void pushTri(std::vector<float>& v, glm::vec3 a, glm::vec3 b, glm::vec3 apex) {
    glm::vec3 n = glm::normalize(glm::cross(b - a, apex - a));
    pushVertex(v, a, n, { 0.0f, 0.0f });
    pushVertex(v, b, n, { 1.0f, 0.0f });
    pushVertex(v, apex, n, { 0.5f, 1.0f });
}

void uploadVAO(int idx, const std::vector<float>& data) {
    glBindVertexArray(VAO[idx]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[idx]);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);                     // 위치
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));   // 법선
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));   // 텍스처 좌표
    glEnableVertexAttribArray(2);
}

void InitBuffer() {
    glGenVertexArrays(3, VAO);
    glGenBuffers(3, VBO);

    // --- 육면체: 면 순서 Front, Back, Top, Bottom, Right, Left (면 i → textures[i])
    std::vector<float> cube;
    pushQuad(cube, { 0, 0, 0.5f }, { 0, 0, 1 }, { 1, 0, 0 }, { 0, 1, 0 });   // Front  (A)
    pushQuad(cube, { 0, 0,-0.5f }, { 0, 0,-1 }, { -1, 0, 0 }, { 0, 1, 0 });   // Back   (B)
    pushQuad(cube, { 0, 0.5f, 0 }, { 0, 1, 0 }, { 1, 0, 0 }, { 0, 0,-1 });   // Top    (C)
    pushQuad(cube, { 0,-0.5f, 0 }, { 0,-1, 0 }, { 1, 0, 0 }, { 0, 0, 1 });   // Bottom (D)
    pushQuad(cube, { 0.5f, 0, 0 }, { 1, 0, 0 }, { 0, 0,-1 }, { 0, 1, 0 });   // Right  (E)
    pushQuad(cube, { -0.5f, 0, 0 }, { -1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 });   // Left   (F)
    cubeVertexCount = (int)cube.size() / 8;   // 36
    uploadVAO(0, cube);

    // --- 사각뿔: 옆면 4개(정점 3개씩) + 바닥(정점 6개)
    std::vector<float> pyr;
    glm::vec3 apex(0.0f, 0.5f, 0.0f);
    glm::vec3 p0(-0.5f, -0.5f, 0.5f), p1(0.5f, -0.5f, 0.5f), p2(0.5f, -0.5f, -0.5f), p3(-0.5f, -0.5f, -0.5f);
    pushTri(pyr, p0, p1, apex);   // 앞   (A)
    pushTri(pyr, p1, p2, apex);   // 오른쪽 (B)
    pushTri(pyr, p2, p3, apex);   // 뒤   (C)
    pushTri(pyr, p3, p0, apex);   // 왼쪽  (D)
    pushQuad(pyr, { 0,-0.5f, 0 }, { 0,-1, 0 }, { 1, 0, 0 }, { 0, 0, 1 });   // 바닥 (E)
    pyramidVertexCount = (int)pyr.size() / 8;  // 18
    uploadVAO(1, pyr);

    // --- 좌표축 (텍스처 없이 선으로)
    std::vector<float> axis;
    glm::vec3 up(0, 1, 0);
    pushVertex(axis, { -2, 0, 0 }, up, { 0, 0 }); pushVertex(axis, { 2, 0, 0 }, up, { 0, 0 });
    pushVertex(axis, { 0, -2, 0 }, up, { 0, 0 }); pushVertex(axis, { 0, 2, 0 }, up, { 0, 0 });
    pushVertex(axis, { 0, 0, -2 }, up, { 0, 0 }); pushVertex(axis, { 0, 0, 2 }, up, { 0, 0 });
    uploadVAO(2, axis);
}

// ======================= 텍스처 =======================
void InitTexture() {
    stbi_set_flip_vertically_on_load(true);   // 이미지는 위→아래로 저장돼 있으니 뒤집어서 (0,0)=왼쪽 아래로 맞춤
    glGenTextures(TEX_COUNT, textures);

    for (int i = 0; i < TEX_COUNT; ++i) {
        glBindTexture(GL_TEXTURE_2D, textures[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        int width, height, nrChannels;
        // 마지막 인자 4: jpg든 png든 항상 RGBA 4채널로 받음 → 행 정렬(4바이트) 문제 없이 GL_RGBA로 올릴 수 있음
        unsigned char* data = stbi_load(texFiles[i], &width, &height, &nrChannels, 4);
        if (data == nullptr) {
            std::cerr << "이미지 로딩 실패: " << texFiles[i] << std::endl;
            continue;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
}

// ======================= 입력 =======================
void resetAll() {
    shapeMode = 0;
    rotX = initRotX; rotY = initRotY;
    animX = false; xDir = 1.0f;
    animY = false; yDir = 1.0f;
}

// 같은 방향 키 → 정지, 다른 방향 키 → 방향 전환 / 시작
void toggleDirAnim(bool& anim, float& dir, float newDir) {
    if (anim && dir == newDir) anim = false;
    else { anim = true; dir = newDir; }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    bool isShift = (mods & GLFW_MOD_SHIFT) != 0;

    switch (key) {
    case GLFW_KEY_C: shapeMode = 0; break;
    case GLFW_KEY_P: shapeMode = 1; break;
    case GLFW_KEY_X: toggleDirAnim(animX, xDir, isShift ? -1.0f : 1.0f); break;
    case GLFW_KEY_Y: toggleDirAnim(animY, yDir, isShift ? -1.0f : 1.0f); break;
    case GLFW_KEY_S: resetAll(); break;
    case GLFW_KEY_Q: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
    }
}

void update(float deltaTime) {
    if (animX) rotX += xDir * 60.0f * deltaTime;
    if (animY) rotY += yDir * 60.0f * deltaTime;
}

// ======================= 그리기 =======================
void drawScene(float deltaTime) {
    update(deltaTime);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.95f, 0.95f, 0.95f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);

    // 1. 좌표축: 텍스처·조명 없이 (ambient 0.3 * objectColor)
    glm::mat4 identity(1.0f);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(identity));
    glUniform1i(useTextureLoc, 0);
    glUniform1i(lightOnLoc, 0);
    glUniform3f(objectColorLoc, 1.5f, 1.5f, 1.5f);   // 0.3 * 1.5 = 0.45 회색
    glBindVertexArray(VAO[2]);
    glDrawArrays(GL_LINES, 0, 6);

    // 2. 도형: 텍스처 + 조명
    glm::mat4 model(1.0f);
    model = glm::rotate(model, glm::radians(rotX), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotY), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1i(useTextureLoc, 1);
    glUniform1i(lightOnLoc, 1);

    glActiveTexture(GL_TEXTURE0);   // 텍스처 유닛 0 (샘플러 outTexture가 참조)

    if (shapeMode == 0) {
        // 육면체: 면(정점 6개)마다 다른 텍스처를 바인딩해서 그림
        glBindVertexArray(VAO[0]);
        for (int face = 0; face < 6; ++face) {
            glBindTexture(GL_TEXTURE_2D, textures[face]);
            glDrawArrays(GL_TRIANGLES, face * 6, 6);
        }
    }
    else {
        // 사각뿔: 옆면 4개(정점 3개씩, first = 0,3,6,9) + 바닥(정점 6개, first = 12)
        glBindVertexArray(VAO[1]);
        for (int face = 0; face < 4; ++face) {
            glBindTexture(GL_TEXTURE_2D, textures[face]);
            glDrawArrays(GL_TRIANGLES, face * 3, 3);
        }
        glBindTexture(GL_TEXTURE_2D, textures[4]);
        glDrawArrays(GL_TRIANGLES, 12, 6);
    }
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 30", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    InitShader();
    InitBuffer();
    InitTexture();

    glfwSetKeyCallback(window, key_callback);

    float lastTime = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        drawScene(deltaTime);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}