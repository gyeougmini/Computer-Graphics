#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cmath>
#include <random>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// ============================================================
// 실습 29 - 눈 내리는 장면 (조명 2)
//  s   : 눈 내리기 / 멈추기
//  r   : 조명이 y축 기준 공전 / 멈춤
//  n/f : 조명이 중앙으로 가까워지기 / 멀어지기
//  +/- : 조명 세기 증가 / 감소
//  q   : 종료
// ============================================================

GLuint shaderProgramID;
GLint locModel, locView, locProj, locColor, locLightPos, locLightColor, locViewPos, locLightOn;

// --- 메쉬 (정점마다 위치3 + 법선3) ---
struct Mesh {
    GLuint vao = 0, vbo = 0;
    GLsizei count = 0;
    GLenum mode = GL_TRIANGLES;
};
Mesh cubeMesh, sphereMesh, pyramidMesh, circleMesh;

// --- 카메라 ---
glm::vec3 cameraPos = glm::vec3(0.0f, 10.0f, 22.0f);

// --- 조명 상태 ---
bool  isLightRotating = false;   // r
float lightAngle = 0.0f;         // 0도 = +X쪽(화면 오른쪽)
float lightRadius = 6.0f;        // n/f
float lightHeight = 6.0f;        // 우측 '상단'
float lightIntensity = 1.0f;     // +/-
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);

// --- 행성 ---
struct Planet {
    float orbitRadius;
    float size;
    float speed;        // 도/초
    float angle;
    glm::vec3 tiltAxis; // 궤도 기울기 축
    float tiltDeg;      // 궤도 기울기 각도
    glm::vec3 color;
};
Planet planets[3] = {
    // 수성: 작고, 가깝고, 빠름. 궤도 기울기 없음
    { 2.5f, 0.25f, 90.0f, 0.0f,   glm::vec3(0, 0, 1), 0.0f,   glm::vec3(0.6f, 0.6f, 0.6f) },
    // 금성: 중간, z축 기준 25도 기울어진 궤도
    { 4.0f, 0.40f, 55.0f, 120.0f, glm::vec3(0, 0, 1), 25.0f,  glm::vec3(0.9f, 0.7f, 0.3f) },
    // 지구: 크고, 멀고, 느림. x축 기준 -20도 기울어진 궤도
    { 5.8f, 0.50f, 35.0f, 240.0f, glm::vec3(1, 0, 0), -20.0f, glm::vec3(0.2f, 0.4f, 0.9f) },
};
const glm::vec3 ORBIT_CENTER = glm::vec3(0.0f, 1.2f, 0.0f); // 피라미드 중간 높이

// --- 눈 ---
const int   SNOW_COUNT = 400;
const float FLOOR_HALF = 10.0f;   // 바닥 크기의 절반 (20 x 20)
const float SNOW_TOP = 14.0f;
struct Snow {
    glm::vec3 pos;
    float speed;
};
std::vector<Snow> snows(SNOW_COUNT);
bool isSnowing = false;   // s: 내리기/멈추기
bool snowStarted = false; // 처음 s 누르기 전에는 눈을 그리지 않음

std::mt19937 rng(std::random_device{}());
float randf(float a, float b) {
    std::uniform_real_distribution<float> d(a, b);
    return d(rng);
}

void respawnSnow(Snow& s, bool randomHeight) {
    s.pos.x = randf(-FLOOR_HALF, FLOOR_HALF);
    s.pos.z = randf(-FLOOR_HALF, FLOOR_HALF);
    s.pos.y = randomHeight ? randf(0.0f, SNOW_TOP) : SNOW_TOP + randf(0.0f, 2.0f);
    s.speed = randf(1.0f, 4.0f);   // 랜덤 낙하 속도
}

// ============================================================
// 셰이더 / 버퍼 초기화
// ============================================================
GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, NULL, log);
        std::cerr << "Shader compile error:\n" << log << std::endl;
    }
    return s;
}

std::string filetobuf(const char* file) {
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open()) {
        std::cerr << "Cannot open " << file << std::endl;
        return "";
    }
    return std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
}

void InitShader() {
    std::string vStr = filetobuf("vertex26.glsl");
    std::string fStr = filetobuf("fragment26.glsl");
    GLuint vs = compileShader(GL_VERTEX_SHADER, vStr.c_str());
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fStr.c_str());
    shaderProgramID = glCreateProgram();
    glAttachShader(shaderProgramID, vs);
    glAttachShader(shaderProgramID, fs);
    glLinkProgram(shaderProgramID);
    glDeleteShader(vs);
    glDeleteShader(fs);

    locModel = glGetUniformLocation(shaderProgramID, "model");
    locView = glGetUniformLocation(shaderProgramID, "view");
    locProj = glGetUniformLocation(shaderProgramID, "projection");
    locColor = glGetUniformLocation(shaderProgramID, "objectColor");
    locLightPos = glGetUniformLocation(shaderProgramID, "lightPos");
    locLightColor = glGetUniformLocation(shaderProgramID, "lightColor");
    locViewPos = glGetUniformLocation(shaderProgramID, "viewPos");
    locLightOn = glGetUniformLocation(shaderProgramID, "lightOn");
}

Mesh createMesh(const std::vector<float>& data, GLenum mode) {
    Mesh m;
    m.mode = mode;
    m.count = (GLsizei)(data.size() / 6);
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), data.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    return m;
}

void pushVertex(std::vector<float>& v, glm::vec3 p, glm::vec3 n) {
    v.insert(v.end(), { p.x, p.y, p.z, n.x, n.y, n.z });
}

// 삼각형 하나를 면 법선(flat)으로 추가
void pushTriangle(std::vector<float>& v, glm::vec3 a, glm::vec3 b, glm::vec3 c) {
    glm::vec3 n = glm::normalize(glm::cross(b - a, c - a));
    pushVertex(v, a, n); pushVertex(v, b, n); pushVertex(v, c, n);
}

// 단위 육면체 (-0.5 ~ 0.5)
std::vector<float> makeCube() {
    std::vector<float> v;
    glm::vec3 p[8] = {
        { -0.5f,-0.5f, 0.5f },{ 0.5f,-0.5f, 0.5f },{ 0.5f, 0.5f, 0.5f },{ -0.5f, 0.5f, 0.5f },
        { -0.5f,-0.5f,-0.5f },{ 0.5f,-0.5f,-0.5f },{ 0.5f, 0.5f,-0.5f },{ -0.5f, 0.5f,-0.5f }
    };
    int faces[6][4] = {
        { 0,1,2,3 }, // front  +z
        { 5,4,7,6 }, // back   -z
        { 3,2,6,7 }, // top    +y
        { 4,5,1,0 }, // bottom -y
        { 1,5,6,2 }, // right  +x
        { 4,0,3,7 }  // left   -x
    };
    for (auto& f : faces) {
        pushTriangle(v, p[f[0]], p[f[1]], p[f[2]]);
        pushTriangle(v, p[f[0]], p[f[2]], p[f[3]]);
    }
    return v;
}

// 사각뿔: 밑면 -0.5~0.5 (y=0), 꼭짓점 (0,1,0)
std::vector<float> makePyramid() {
    std::vector<float> v;
    glm::vec3 apex(0.0f, 1.0f, 0.0f);
    glm::vec3 b0(-0.5f, 0.0f, 0.5f), b1(0.5f, 0.0f, 0.5f);
    glm::vec3 b2(0.5f, 0.0f, -0.5f), b3(-0.5f, 0.0f, -0.5f);
    pushTriangle(v, b0, b1, apex); // 앞
    pushTriangle(v, b1, b2, apex); // 오른쪽
    pushTriangle(v, b2, b3, apex); // 뒤
    pushTriangle(v, b3, b0, apex); // 왼쪽
    pushTriangle(v, b0, b3, b2);   // 밑면 (아래를 향하게)
    pushTriangle(v, b0, b2, b1);
    return v;
}

// 반지름 1 구 (구는 위치 = 법선이라 부드럽게 보임)
std::vector<float> makeSphere(int stacks, int slices) {
    std::vector<float> v;
    const float PI = 3.14159265f;
    auto pt = [&](int i, int j) {
        float phi = PI * i / stacks;          // 0 ~ PI
        float theta = 2.0f * PI * j / slices; // 0 ~ 2PI
        return glm::vec3(sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta));
        };
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            glm::vec3 a = pt(i, j), b = pt(i + 1, j), c = pt(i + 1, j + 1), d = pt(i, j + 1);
            pushVertex(v, a, a); pushVertex(v, c, c); pushVertex(v, b, b);
            pushVertex(v, a, a); pushVertex(v, d, d); pushVertex(v, c, c);
        }
    }
    return v;
}

// 궤도선용 반지름 1 원 (xz 평면)
std::vector<float> makeCircle(int seg) {
    std::vector<float> v;
    for (int i = 0; i < seg; ++i) {
        float t = 2.0f * 3.14159265f * i / seg;
        pushVertex(v, glm::vec3(cos(t), 0.0f, sin(t)), glm::vec3(0, 1, 0));
    }
    return v;
}

void InitBuffer() {
    cubeMesh = createMesh(makeCube(), GL_TRIANGLES);
    pyramidMesh = createMesh(makePyramid(), GL_TRIANGLES);
    sphereMesh = createMesh(makeSphere(20, 30), GL_TRIANGLES);
    circleMesh = createMesh(makeCircle(96), GL_LINE_LOOP);
}

// ============================================================
// 입력 / 갱신
// ============================================================
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    switch (key) {
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    case GLFW_KEY_S:
        if (action != GLFW_PRESS) break;     // 꾹 누를 때 토글이 반복되지 않게
        isSnowing = !isSnowing;
        snowStarted = true;
        break;
    case GLFW_KEY_R:
        if (action != GLFW_PRESS) break;
        isLightRotating = !isLightRotating;
        break;
    case GLFW_KEY_N:   // 가까워지기
        lightRadius -= 0.5f;
        if (lightRadius < 1.0f) lightRadius = 1.0f;
        break;
    case GLFW_KEY_F:   // 멀어지기
        lightRadius += 0.5f;
        if (lightRadius > 18.0f) lightRadius = 18.0f;
        break;
    case GLFW_KEY_EQUAL:      // '+' (= 키, Shift 여부 무관)
    case GLFW_KEY_KP_ADD:
        lightIntensity += 0.1f;
        if (lightIntensity > 2.0f) lightIntensity = 2.0f;
        break;
    case GLFW_KEY_MINUS:
    case GLFW_KEY_KP_SUBTRACT:
        lightIntensity -= 0.1f;
        if (lightIntensity < 0.0f) lightIntensity = 0.0f;
        break;
    }
}

void update(float dt) {
    // 행성 공전
    for (auto& p : planets) {
        p.angle += p.speed * dt;
        if (p.angle >= 360.0f) p.angle -= 360.0f;
    }

    // 조명 공전 (y축 기준)
    if (isLightRotating) {
        lightAngle += 40.0f * dt;
        if (lightAngle >= 360.0f) lightAngle -= 360.0f;
    }

    // 눈 낙하: 바닥(y=0)에 닿으면 다시 위에서 시작
    if (isSnowing) {
        for (auto& s : snows) {
            s.pos.y -= s.speed * dt;
            if (s.pos.y <= 0.05f) respawnSnow(s, false);
        }
    }
}

glm::vec3 getLightPos() {
    float rad = glm::radians(lightAngle);
    return glm::vec3(lightRadius * cos(rad), lightHeight, lightRadius * sin(rad));
}

// ============================================================
// 그리기
// ============================================================
void drawMesh(const Mesh& m, const glm::mat4& model, glm::vec3 color, bool unlit = false) {
    glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(model));
    // fragment26은 조명이 꺼지면 0.3 * objectColor만 출력하므로, 원하는 색이 그대로 나오게 0.3으로 나눠서 넘김
    glm::vec3 c = unlit ? color / 0.3f : color;
    glUniform3fv(locColor, 1, glm::value_ptr(c));
    glUniform1i(locLightOn, unlit ? 0 : 1);
    glBindVertexArray(m.vao);
    glDrawArrays(m.mode, 0, m.count);
}

// 행성 궤도 기울기 변환 (궤도 중심 -> 기울기)
glm::mat4 orbitBase(const Planet& p) {
    glm::mat4 m = glm::translate(glm::mat4(1.0f), ORBIT_CENTER);
    if (p.tiltDeg != 0.0f) m = glm::rotate(m, glm::radians(p.tiltDeg), p.tiltAxis);
    return m;
}

void drawScene(float dt) {
    update(dt);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.05f, 0.07f, 0.12f, 1.0f); // 겨울 밤하늘
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    glm::mat4 view = glm::lookAt(cameraPos, glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    glm::vec3 lightPos = getLightPos();

    glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(proj));
    glUniform3fv(locLightPos, 1, glm::value_ptr(lightPos));
    glm::vec3 scaledLight = lightColor * lightIntensity; // +/- 세기는 lightColor에 곱해서 전달
    glUniform3fv(locLightColor, 1, glm::value_ptr(scaledLight));
    glUniform3fv(locViewPos, 1, glm::value_ptr(cameraPos));

    // 1. 바닥
    glm::mat4 ground = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f));
    ground = glm::scale(ground, glm::vec3(FLOOR_HALF * 2.0f, 0.2f, FLOOR_HALF * 2.0f));
    drawMesh(cubeMesh, ground, glm::vec3(0.75f, 0.78f, 0.82f));

    // 2. 중앙 피라미드
    glm::mat4 pyramid = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.5f, 2.0f));
    drawMesh(pyramidMesh, pyramid, glm::vec3(0.85f, 0.35f, 0.25f));

    // 3. 행성 + 궤도선
    for (const auto& p : planets) {
        glm::mat4 base = orbitBase(p);

        glm::mat4 orbit = glm::scale(base, glm::vec3(p.orbitRadius));
        drawMesh(circleMesh, orbit, glm::vec3(0.4f, 0.4f, 0.5f), true);

        glm::mat4 planet = glm::rotate(base, glm::radians(p.angle), glm::vec3(0.0f, 1.0f, 0.0f));
        planet = glm::translate(planet, glm::vec3(p.orbitRadius, 0.0f, 0.0f));
        planet = glm::scale(planet, glm::vec3(p.size));
        drawMesh(sphereMesh, planet, p.color);
    }

    // 4. 조명 객체 (조명 위치에 작은 구, 세기에 따라 밝기 변화)
    glm::mat4 lightModel = glm::translate(glm::mat4(1.0f), lightPos);
    lightModel = glm::scale(lightModel, glm::vec3(0.35f));
    float shown = glm::clamp(0.3f + 0.7f * lightIntensity, 0.0f, 1.0f);
    drawMesh(sphereMesh, lightModel, glm::vec3(1.0f, 1.0f, 0.6f) * shown, true);

    // 5. 눈
    if (snowStarted) {
        for (const auto& s : snows) {
            glm::mat4 m = glm::translate(glm::mat4(1.0f), s.pos);
            m = glm::scale(m, glm::vec3(0.08f));
            drawMesh(sphereMesh, m, glm::vec3(1.0f, 1.0f, 1.0f));
        }
    }
}

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 29 - Snow Scene with Lighting", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    InitShader();
    InitBuffer();
    for (auto& s : snows) respawnSnow(s, true); // 처음엔 하늘 전체에 흩뿌려 둠

    glfwSetKeyCallback(window, key_callback);

    float lastTime = (float)glfwGetTime();
    while (!glfwWindowShouldClose(window)) {
        float now = (float)glfwGetTime();
        float dt = now - lastTime;
        lastTime = now;
        if (dt > 0.1f) dt = 0.1f;

        drawScene(dt);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}