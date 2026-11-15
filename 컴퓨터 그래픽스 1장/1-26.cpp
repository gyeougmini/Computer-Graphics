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
// 조명 큐브는 cubevao를 그대로 재사용 (모양·법선이 같고, 색은 uniform으로 지정)

float xPos = 0.0f, yPos = 0.0f, zPos = 0.0f;
bool isDepthTest = true;
bool isPyramidMode = false;

bool rotateY = false;
float yAngle = 0.0f;


bool rotateR = false;
float camRDir = 1.0f; 
float rAngle = 0.0f;

bool  moveZ = false;        // Z키 토글
float zPhase = 0.0f;        // sin에 넣을 각도 (누적)
float lightRadius = 1.5f;

bool lightOn = true;

// 면별 색 (이제 정점 속성이 아니라 objectColor uniform으로 전달)
const glm::vec3 cubeFaceColors[6] = {
    {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f},
    {1.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 1.0f}
};
const glm::vec3 pyramidBaseColor = { 0.5f, 0.5f, 0.5f };
const glm::vec3 pyramidSideColors[4] = {
    {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f}
};

std::string filetobuf(const char* file) {
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open()) {
        std::cerr << "셰이더 파일 열기 실패: " << file << std::endl;
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
}

// 컴파일 에러를 콘솔에 출력 (에러가 나면 화면이 하얗게만 나오므로 꼭 확인)
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

    GLint ok;
    glGetProgramiv(shaderProgramID, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(shaderProgramID, 1024, NULL, log);
        std::cerr << "프로그램 링크 에러:\n" << log << std::endl;
    }

    glDeleteShader(vShader);
    glDeleteShader(fShader);
}

void InitBuffer() {
    GLfloat cubeShape[36][3] = {
        {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},   // 0: 앞 (+z)
        { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f},
        {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},   // 1: 뒤 (-z)
        { 0.5f, 0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f},
        {-0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},   // 2: 위 (+y)
        { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},   // 3: 아래 (-y)
        { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f,-0.5f},
        { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f},   // 4: 오른쪽 (+x)
        { 0.5f, 0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f},
        {-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},   // 5: 왼쪽 (-x)
        {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}
    };

    // 색 대신 법선 벡터 (location = 1 → vNormal)
    const GLfloat cubeFaceNormals[6][3] = {
        { 0.0f, 0.0f, 1.0f}, { 0.0f, 0.0f,-1.0f},
        { 0.0f, 1.0f, 0.0f}, { 0.0f,-1.0f, 0.0f},
        { 1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}
    };
    GLfloat cubeNormals[36][3];
    for (int i = 0; i < 36; i++) {
        int face = i / 6;
        for (int k = 0; k < 3; k++) cubeNormals[i][k] = cubeFaceNormals[face][k];
    }

    GLfloat pyramid[18][3] = {
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},     // 바닥
        { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f,-0.5f},
        { 0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, { 0.0f, 0.5f, 0.0f},   // 앞 (+z)
        { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, 0.5f}, { 0.0f, 0.5f, 0.0f},  // 오른쪽 (+x)
        {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.0f, 0.5f, 0.0f}, // 뒤 (-z)
        {-0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, -0.5f}, { 0.0f, 0.5f, 0.0f}   // 왼쪽 (-x)
    };

    // 옆면 법선: 바깥 방향 + 위쪽으로 기울어짐. (0, 0.5, 1)을 정규화하면 (0, 0.447, 0.894)
    const GLfloat pyramidFaceNormals[5][3] = {
        { 0.0f,   -1.0f,   0.0f   },   // 바닥
        { 0.0f,    0.447f, 0.894f },   // 앞
        { 0.894f,  0.447f, 0.0f   },   // 오른쪽
        { 0.0f,    0.447f,-0.894f },   // 뒤
        {-0.894f,  0.447f, 0.0f   }    // 왼쪽
    };
    GLfloat pyramidNormals[18][3];
    for (int i = 0; i < 18; i++) {
        int face = (i < 6) ? 0 : 1 + (i - 6) / 3;
        for (int k = 0; k < 3; k++) pyramidNormals[i][k] = pyramidFaceNormals[face][k];
    }

    glGenVertexArrays(1, &cubevao);
    glBindVertexArray(cubevao);
    glGenBuffers(2, cubevbo);

    glBindBuffer(GL_ARRAY_BUFFER, cubevbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeShape), cubeShape, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, cubevbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeNormals), cubeNormals, GL_STATIC_DRAW);
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidNormals), pyramidNormals, GL_STATIC_DRAW);
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

    case GLFW_KEY_H: isDepthTest = !isDepthTest; break;
    case GLFW_KEY_N: isPyramidMode = !isPyramidMode; break;
    case GLFW_KEY_Y: rotateY = !rotateY; break;
    case GLFW_KEY_R: 
        rotateR = !rotateR;
        camRDir = isShift ? -1.0f : 1.0f;
        break;
    case GLFW_KEY_Z: moveZ = !moveZ; break;

    case GLFW_KEY_M: lightOn = !lightOn; break;

    }
}

void UpdateAnimations(float deltaTime) {
    float speedMultiplier = deltaTime * 60.0f;

    if (rotateY) {
        yAngle += 1.0f * speedMultiplier;
        if (yAngle >= 360.0f) yAngle -= 360.0f;
    }

    if (rotateR) { rAngle += camRDir * 45.0f * deltaTime; }

    if (moveZ) {
        zPhase += 2.0f * deltaTime;                    // 속도 (클수록 빨리 왕복)
        lightRadius = 1.5f + 0.5f * sin(zPhase);       // 1.0 ~ 2.0 사이를 왕복
    }
}

void drawScene(GLFWwindow* window, float deltaTime) {
    UpdateAnimations(deltaTime);

    if (isDepthTest) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    glm::vec3 cameraPos(2.5f, 2.0f, 2.5f);
    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

    // 셰이더가 요구하는 uniform 이름 그대로 가져오기
    GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
    GLint projLoc = glGetUniformLocation(shaderProgramID, "projection");
    GLint lightPosLoc = glGetUniformLocation(shaderProgramID, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(shaderProgramID, "lightColor");
    GLint objectColorLoc = glGetUniformLocation(shaderProgramID, "objectColor");
    GLint viewPosLoc = glGetUniformLocation(shaderProgramID, "viewPos");


    glUniform1i(glGetUniformLocation(shaderProgramID, "lightOn"), lightOn);
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);   // 색은 항상 흰색

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));

    if (lightOn) glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    else         glUniform3f(lightColorLoc, 0.0f, 0.0f, 0.0f);

    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

    glm::vec3 center(xPos, yPos, zPos);
    glm::mat4 orbit = glm::rotate(glm::mat4(1.0f), glm::radians(rAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 lightPos = center + glm::vec3(orbit * glm::vec4(lightRadius, 0.0f, 0.0f, 1.0f));

    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));   // ← 셰이더에도 공전한 위치를 넘김
    glUniform3f(lightColorLoc, 1.0f, 1.0f, 1.0f);
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // --- 조명 큐브: 계산된 위치로 옮기고 크기만 줄임 (자전 X)
    glm::mat4 lightModel = glm::mat4(1.0f);
    lightModel = glm::translate(lightModel, lightPos);
    lightModel = glm::scale(lightModel, glm::vec3(0.3f));

    glBindVertexArray(cubevao);
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lightModel));
    glUniform3f(objectColorLoc, 0.5f, 0.5f, 0.5f);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // --- 본체
    glm::mat4 baseModel = glm::mat4(1.0f);
    baseModel = glm::translate(baseModel, glm::vec3(xPos, yPos, zPos));
    baseModel = glm::rotate(baseModel, glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    baseModel = glm::rotate(baseModel, glm::radians(yAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(baseModel));

    if (!isPyramidMode) {
        glBindVertexArray(cubevao);
        for (int i = 0; i < 6; i++) {
            glUniform3fv(objectColorLoc, 1, glm::value_ptr(cubeFaceColors[i]));   // 면마다 색 변경
            glDrawArrays(GL_TRIANGLES, i * 6, 6);
        }
    }
    else {
        glBindVertexArray(pyramidvao);

        glUniform3fv(objectColorLoc, 1, glm::value_ptr(pyramidBaseColor));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        for (int i = 0; i < 4; i++) {
            glUniform3fv(objectColorLoc, 1, glm::value_ptr(pyramidSideColors[i]));
            glDrawArrays(GL_TRIANGLES, 6 + i * 3, 3);
        }
    }
}

int main() {
    srand(static_cast<unsigned int>(time(NULL)));

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 16", nullptr, nullptr);
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

        if (deltaTime > 0.1f) deltaTime = 0.1f;

        drawScene(window, deltaTime);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}