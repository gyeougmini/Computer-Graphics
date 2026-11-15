#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cfloat>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ───────────────────── 전역 ─────────────────────
GLuint shaderProgramID;
GLuint spherevao, spherevbo[2];
GLsizei sphereVertexCount = 0;

// 조명
float lightRadius = 2.0f;     // 중앙 구 중심에서 조명까지 거리
float lightAngle = 0.0f;      // 공전 각도 (0° = 화면 앞쪽 +z)
bool  rotateLight = false;
float lightDir = 1.0f;        // r: +1, R: -1

// c키로 바꿀 조명 색: 흰색(기본) → 빨강 → 초록 → 파랑
const glm::vec3 lightColors[] = {
    {1.0f, 1.0f, 1.0f},
    {1.0f, 0.0f, 0.0f},
    {0.0f, 1.0f, 0.0f},
    {0.0f, 0.0f, 1.0f}
};
const int lightColorCount = sizeof(lightColors) / sizeof(lightColors[0]);
int lightColorIndex = 0;

// 구 3개: 위치, 반지름, 색
struct Sphere { glm::vec3 pos; float radius; glm::vec3 color; };
const Sphere spheres[3] = {
    { { 0.0f, 0.0f, 0.0f}, 0.5f, {1.0f, 0.5f, 0.3f} },   // 중앙
    { {-1.3f, 0.0f, 0.0f}, 0.3f, {0.3f, 0.8f, 0.3f} },   // 왼쪽
    { { 1.3f, 0.0f, 0.0f}, 0.3f, {0.3f, 0.5f, 1.0f} }    // 오른쪽
};

// ───────────────────── 셰이더 ─────────────────────
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

// ───────────────────── OBJ 로더 ─────────────────────
// v / vn / f 를 읽어서 삼각형 단위로 풀어낸 위치·법선 배열을 만든다.
// f는 "v", "v/vt", "v//vn", "v/vt/vn" 형식 모두 처리하고, 사각형 이상은 부채꼴로 쪼갠다.
// 모델 크기가 제각각이라 중심을 원점으로 옮기고 반지름 1이 되도록 정규화한다.
bool LoadOBJ(const char* path, std::vector<glm::vec3>& outPos, std::vector<glm::vec3>& outNormal) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "OBJ 파일 열기 실패: " << path << std::endl;
        return false;
    }

    std::vector<glm::vec3> positions, normals;
    struct Index { int v, vn; };
    std::vector<Index> indices;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();   // CRLF 대비
        std::istringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            glm::vec3 p; ss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        }
        else if (type == "vn") {
            glm::vec3 n; ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (type == "f") {
            std::vector<Index> face;
            std::string token;
            while (ss >> token) {
                Index idx = { 0, 0 };
                size_t s1 = token.find('/');
                idx.v = std::stoi(token.substr(0, s1));
                if (s1 != std::string::npos) {
                    size_t s2 = token.find('/', s1 + 1);
                    if (s2 != std::string::npos && s2 + 1 < token.size())
                        idx.vn = std::stoi(token.substr(s2 + 1));
                }
                // 음수 인덱스(뒤에서부터) 처리, 1-based → 0-based
                idx.v = (idx.v < 0) ? (int)positions.size() + idx.v : idx.v - 1;
                if (idx.vn != 0) idx.vn = (idx.vn < 0) ? (int)normals.size() + idx.vn : idx.vn - 1;
                else idx.vn = -1;
                face.push_back(idx);
            }
            for (size_t i = 1; i + 1 < face.size(); i++) {   // 부채꼴 삼각형 분할
                indices.push_back(face[0]);
                indices.push_back(face[i]);
                indices.push_back(face[i + 1]);
            }
        }
    }

    // 바운딩 박스로 중심과 크기 구하기
    glm::vec3 mn(FLT_MAX), mx(-FLT_MAX);
    for (auto& p : positions) { mn = glm::min(mn, p); mx = glm::max(mx, p); }
    glm::vec3 center = (mn + mx) * 0.5f;
    float halfSize = glm::max(glm::max(mx.x - mn.x, mx.y - mn.y), mx.z - mn.z) * 0.5f;

    for (auto& idx : indices) {
        glm::vec3 p = (positions[idx.v] - center) / halfSize;   // 반지름 1로 정규화
        outPos.push_back(p);
        // 법선이 없으면 구니까 위치 방향을 법선으로 사용
        outNormal.push_back(idx.vn >= 0 ? glm::normalize(normals[idx.vn]) : glm::normalize(p));
    }

    std::cout << path << ": 삼각형 " << indices.size() / 3 << "개 로드" << std::endl;
    return true;
}

// ───────────────────── 버퍼 ─────────────────────
void InitBuffer() {
    std::vector<glm::vec3> pos, normal;
    LoadOBJ("sphere18.obj", pos, normal);
    sphereVertexCount = (GLsizei)pos.size();

    glGenVertexArrays(1, &spherevao);
    glBindVertexArray(spherevao);
    glGenBuffers(2, spherevbo);

    glBindBuffer(GL_ARRAY_BUFFER, spherevbo[0]);
    glBufferData(GL_ARRAY_BUFFER, pos.size() * sizeof(glm::vec3), pos.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);   // location 0: vPos
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, spherevbo[1]);
    glBufferData(GL_ARRAY_BUFFER, normal.size() * sizeof(glm::vec3), normal.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);   // location 1: vNormal
    glEnableVertexAttribArray(1);
}

// ───────────────────── 입력 ─────────────────────
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;
    bool isShift = (mods & GLFW_MOD_SHIFT) != 0;

    switch (key) {
    case GLFW_KEY_Q:
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

    case GLFW_KEY_C:   // 조명 색 순환
        lightColorIndex = (lightColorIndex + 1) % lightColorCount;
        break;

    case GLFW_KEY_R: { // r: 양 방향, R: 음 방향 (같은 키를 다시 누르면 정지)
        float dir = isShift ? -1.0f : 1.0f;
        if (rotateLight && lightDir == dir) rotateLight = false;
        else { rotateLight = true; lightDir = dir; }
        break;
    }
    }
}

void UpdateAnimations(float deltaTime) {
    if (rotateLight) {
        lightAngle += lightDir * 45.0f * deltaTime;   // 초당 45°
        if (lightAngle >= 360.0f) lightAngle -= 360.0f;
        if (lightAngle < 0.0f)    lightAngle += 360.0f;
    }
}

// ───────────────────── 그리기 ─────────────────────
void drawScene(float deltaTime) {
    UpdateAnimations(deltaTime);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    // 카메라: 정면 위쪽에서 중앙 구를 바라봄
    glm::vec3 cameraPos(0.0f, 2.0f, 6.0f);
    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);

    GLint modelLoc = glGetUniformLocation(shaderProgramID, "model");
    GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
    GLint projLoc = glGetUniformLocation(shaderProgramID, "projection");
    GLint lightPosLoc = glGetUniformLocation(shaderProgramID, "lightPos");
    GLint lightColorLoc = glGetUniformLocation(shaderProgramID, "lightColor");
    GLint objectColorLoc = glGetUniformLocation(shaderProgramID, "objectColor");
    GLint viewPosLoc = glGetUniformLocation(shaderProgramID, "viewPos");
    GLint lightOnLoc = glGetUniformLocation(shaderProgramID, "lightOn");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
    glUniform1i(lightOnLoc, 1);   // 셰이더에 lightOn이 있으면 항상 켜둠 (없으면 location -1이라 무시됨)

    // 조명 위치: 중앙 구 중심에서 앞쪽(+z)으로 lightRadius만큼, y축 기준 lightAngle 회전
    glm::vec3 center = spheres[0].pos;
    glm::mat4 orbit = glm::rotate(glm::mat4(1.0f), glm::radians(lightAngle), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::vec3 lightPos = center + glm::vec3(orbit * glm::vec4(0.0f, 0.5f, lightRadius, 1.0f));
    glm::vec3 lightColor = lightColors[lightColorIndex];

    glUniform3fv(lightPosLoc, 1, glm::value_ptr(lightPos));
    glUniform3fv(lightColorLoc, 1, glm::value_ptr(lightColor));

    glBindVertexArray(spherevao);

    // 구 3개
    for (int i = 0; i < 3; i++) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), spheres[i].pos);
        model = glm::scale(model, glm::vec3(spheres[i].radius));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(objectColorLoc, 1, glm::value_ptr(spheres[i].color));
        glDrawArrays(GL_TRIANGLES, 0, sphereVertexCount);
    }

    // 조명 표시용 작은 구 (조명 색으로)
    glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), lightPos);
    lightModel = glm::scale(lightModel, glm::vec3(0.1f));
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lightModel));
    glUniform3fv(objectColorLoc, 1, glm::value_ptr(lightColor));
    glDrawArrays(GL_TRIANGLES, 0, sphereVertexCount);
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 27", nullptr, nullptr);
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

        drawScene(deltaTime);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}