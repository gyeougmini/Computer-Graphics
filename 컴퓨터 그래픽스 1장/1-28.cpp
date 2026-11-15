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
GLuint cubeVAO, cubeVBO[2]; // 0: 위치, 1: 법선

// uniform 위치 (InitShader 후 한 번만 구함)
GLint modelLoc, viewLoc, projLoc, objectColorLoc;
GLint lightPosLoc, lightColorLoc, viewPosLoc, lightOnLoc;

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

bool animCamR = false; float camRDir = 1.0f; float camRAngle = 0.0f; // r/R: 월드 중심 공전

// --- 조명 변수 ---
const glm::vec3 lightStartPos = glm::vec3(3.0f, 3.0f, 2.0f);   // 처음 위치: 화면 우측 상단
bool  lightOn = true;                                         // m
bool  animLight = false; float lightDir = 1.0f; float lightAngle = 0.0f;   // y/Y: 공전

const glm::vec3 lightColors[] = {        // c: 흰색 → 빨강 → 초록 → 파랑
    {1.0f, 1.0f, 1.0f},
    {1.0f, 0.3f, 0.3f},
    {0.3f, 1.0f, 0.3f},
    {0.3f, 0.3f, 1.0f}
};
const int lightColorCount = sizeof(lightColors) / sizeof(lightColors[0]);
int lightColorIndex = 0;

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
    // 조명 셰이더 (실습 26~27에서 쓰던 파일 그대로)
    std::string vStr = filetobuf("vertex26.glsl");
    std::string fStr = filetobuf("fragment26.glsl");
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
    lightOnLoc = glGetUniformLocation(shaderProgramID, "lightOn");   // 없으면 -1 → 무시됨
}

void InitBuffer() {
    // 면 순서(Front, Back, Top, Bottom, Right, Left)에 맞춘 법선
    const float faceNormals[6][3] = {
        { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f,-1.0f},
        { 0.0f, 1.0f, 0.0f}, { 0.0f,-1.0f, 0.0f},
        { 1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}
    };
    float cubeNormals[36 * 3];
    for (int i = 0; i < 36; i++)
        for (int k = 0; k < 3; k++)
            cubeNormals[i * 3 + k] = faceNormals[i / 6][k];

    glGenVertexArrays(1, &cubeVAO);
    glBindVertexArray(cubeVAO);
    glGenBuffers(2, cubeVBO);

    // Location 0: 정점 위치 (vPos)
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glEnableVertexAttribArray(0);

    // Location 1: 정점 법선 (vNormal) — 색은 이제 objectColor uniform으로 전달
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeNormals), cubeNormals, GL_STATIC_DRAW);
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
    animCamR = false; camRAngle = 0.0f;

    lightOn = true; lightColorIndex = 0;
    animLight = false; lightAngle = 0.0f;
}

// 같은 방향 키 → 정지, 다른 방향 키 → 방향 전환 / 시작
void toggleDirAnim(bool& anim, float& dir, float newDir) {
    if (anim && dir == newDir) anim = false;
    else { anim = true; dir = newDir; }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;
    bool isShift = (mods & GLFW_MOD_SHIFT) != 0;

    // 누르고 있으면 계속 움직이는 키 (REPEAT 허용)
    switch (key) {
    case GLFW_KEY_UP:    tankPos.z -= 0.1f; return;
    case GLFW_KEY_DOWN:  tankPos.z += 0.1f; return;
    case GLFW_KEY_LEFT:  tankPos.x -= 0.1f; return;
    case GLFW_KEY_RIGHT: tankPos.x += 0.1f; return;

        // w/a/s/d: 카메라를 월드 x/z축으로 평행 이동 (바라보는 점도 같이 이동)
    case GLFW_KEY_W: cameraPos.z -= 0.2f; cameraTarget.z -= 0.2f; return;
    case GLFW_KEY_S: cameraPos.z += 0.2f; cameraTarget.z += 0.2f; return;
    case GLFW_KEY_A: cameraPos.x -= 0.2f; cameraTarget.x -= 0.2f; return;
    case GLFW_KEY_D: cameraPos.x += 0.2f; cameraTarget.x += 0.2f; return;
    }

    // 토글 키는 처음 누를 때만 (누르고 있어도 깜빡이지 않게)
    if (action != GLFW_PRESS) return;

    switch (key) {
    case GLFW_KEY_Q: glfwSetWindowShouldClose(window, GLFW_TRUE); break;

        // --- 실습 28: 조명 ---
    case GLFW_KEY_M: lightOn = !lightOn; break;
    case GLFW_KEY_C: lightColorIndex = (lightColorIndex + 1) % lightColorCount; break;
    case GLFW_KEY_Y: toggleDirAnim(animLight, lightDir, isShift ? -1.0f : 1.0f); break;

        // --- 실습 28: 카메라 공전 ---
    case GLFW_KEY_R: toggleDirAnim(animCamR, camRDir, isShift ? -1.0f : 1.0f); break;

        // --- 실습 19 탱크 명령 (겹치는 키는 옮김) ---
    case GLFW_KEY_B: animD = !animD; break;   // 아래 몸체 크기 (원래 d)
    case GLFW_KEY_T: animT = !animT; break;
    case GLFW_KEY_L:
    case GLFW_KEY_1:
        if (!animSwap) { animSwap = true; swapProgress = 0.0f; }
        break;
    case GLFW_KEY_G: animG = !animG; break;
    case GLFW_KEY_P: animP = !animP; break;
    case GLFW_KEY_O: randomizeAllColors(); break;
    case GLFW_KEY_V: resetAll(); break;       // 초기화 (원래 c)
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

    if (animCamR)  camRAngle += camRDir * 45.0f * deltaTime;
    if (animLight) lightAngle += lightDir * 45.0f * deltaTime;
}

// 색은 objectColor uniform으로, 변환은 model 행렬로 넘김 (view/projection은 프레임당 한 번)
void drawCube(const glm::mat4& model, glm::vec3 color) {
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(objectColorLoc, 1, glm::value_ptr(color));
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void drawScene(GLFWwindow* window, float deltaTime) {
    update(deltaTime);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.12f, 0.12f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    // --- 카메라: 월드 중심 y축 기준 공전 (위치와 바라보는 점을 같이 회전)
    glm::mat4 camRot = glm::rotate(glm::mat4(1.0f), glm::radians(camRAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 currentCamPos = glm::vec3(camRot * glm::vec4(cameraPos, 1.0f));
    glm::vec3 currentCamTarget = glm::vec3(camRot * glm::vec4(cameraTarget, 1.0f));

    glm::mat4 view = glm::lookAt(currentCamPos, currentCamTarget, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(currentCamPos));

    // --- 조명: 화면 중심 y축 기준 공전
    glm::mat4 lightRot = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 lightPos = glm::vec3(lightRot * glm::vec4(lightStartPos, 1.0f));
    glm::vec3 lightColor = lightColors[lightColorIndex];

    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    // 꺼지면 lightOn = 0, 조명색 = 검정 (셰이더에 lightOn이 없어도 꺼지도록 둘 다 보냄)
    glUniform1i(lightOnLoc, lightOn ? 1 : 0);
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightOn ? lightColor : glm::vec3(0.0f)));

    glBindVertexArray(cubeVAO);

    // 0. 조명 표시용 작은 큐브 (켜져 있으면 조명색, 꺼지면 회색)
    glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), lightPos);
    lightModel = glm::scale(lightModel, glm::vec3(0.3f));
    drawCube(lightModel, lightOn ? lightColor : glm::vec3(0.3f));

    // 1. 바닥
    glm::mat4 floorModel = glm::mat4(1.0f);
    floorModel = glm::translate(floorModel, glm::vec3(0.0f, -0.05f, 0.0f));
    floorModel = glm::scale(floorModel, glm::vec3(12.0f, 0.1f, 12.0f));
    drawCube(floorModel, cFloor);

    // 2. 탱크 기준 루트
    glm::mat4 tankRoot = glm::mat4(1.0f);
    tankRoot = glm::translate(tankRoot, tankPos);
    tankRoot = glm::rotate(tankRoot, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // 3. 아래 몸체 (2개 - 동일한 cLower 색상)
    float lowerHeight = 0.6f * lowerScaleY;
    glm::mat4 lowerLeft = glm::translate(tankRoot, glm::vec3(-0.65f, lowerHeight * 0.5f, 0.0f));
    lowerLeft = glm::scale(lowerLeft, glm::vec3(0.8f, lowerHeight, 2.0f));
    drawCube(lowerLeft, cLower);

    glm::mat4 lowerRight = glm::translate(tankRoot, glm::vec3(0.65f, lowerHeight * 0.5f, 0.0f));
    lowerRight = glm::scale(lowerRight, glm::vec3(0.8f, lowerHeight, 2.0f));
    drawCube(lowerRight, cLower);

    // 4. 중앙 몸체 (y축 회전 - t키)
    glm::mat4 midModel = glm::translate(tankRoot, glm::vec3(0.0f, lowerHeight + 0.15f, 0.0f));
    midModel = glm::rotate(midModel, glm::radians(midRotY), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 midDraw = glm::scale(midModel, glm::vec3(1.8f, 0.3f, 1.6f));
    drawCube(midDraw, cMid);

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
        drawCube(upperDraw, cUpper);

        // 포신 2개 (동일한 cBarrel 색상)
        float bRot = (i == 0) ? barrelRotY : -barrelRotY;
        glm::mat4 barrelModel = glm::translate(upperModel, glm::vec3(0.0f, 0.0f, -0.6f));
        barrelModel = glm::rotate(barrelModel, glm::radians(bRot), glm::vec3(0.0f, 1.0f, 0.0f));
        barrelModel = glm::scale(barrelModel, glm::vec3(0.12f, 0.12f, 0.6f));
        drawCube(barrelModel, cBarrel);

        // 깃대 2개 (동일한 cFlag 색상)
        float fRot = (i == 0) ? flagRotX : -flagRotX;
        glm::mat4 flagModel = glm::translate(upperModel, glm::vec3(0.0f, 0.45f, 0.0f));
        flagModel = glm::rotate(flagModel, glm::radians(fRot), glm::vec3(1.0f, 0.0f, 0.0f));
        flagModel = glm::scale(flagModel, glm::vec3(0.08f, 0.5f, 0.08f));
        drawCube(flagModel, cFlag);
    }
}

int main() {
    srand(static_cast<unsigned int>(time(NULL))); // 시드 초기화

    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 28", nullptr, nullptr);
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