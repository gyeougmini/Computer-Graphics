#include <GL/glew.h>
#include <GL/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>

// ==========================================
// 1. 셰이더 소스
// ==========================================

// 버텍스 셰이더: 크기 → 회전 → 이동 순서로 적용
//  - uAngle: 삼각형이 진행 방향을 바라보도록 회전시키는 각도 (라디안)
//  - mat2는 열 우선(column-major)이라 mat2(c, s, -s, c)가 회전행렬이 된다
const char* vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 vPos;\n"
"uniform vec2 uTranslation;\n"
"uniform float uScale;\n"
"uniform float uAngle;\n"
"void main()\n"
"{\n"
"    vec2 p = vPos.xy * uScale;\n"
"    float c = cos(uAngle);\n"
"    float s = sin(uAngle);\n"
"    mat2 rot = mat2(c, s, -s, c);\n"
"    vec2 r = rot * p;\n"
"    gl_Position = vec4(r + uTranslation, vPos.z, 1.0);\n"
"}\n";

// 프래그먼트 셰이더: uniform 색상 그대로 출력
const char* fragmentShaderSource =
"#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec4 uColor;\n"
"void main()\n"
"{\n"
"    FragColor = uColor;\n"
"}\n";


#define PI_F 3.14159265f
#define TRAIL_MAX 1500   // 스파이럴 경로로 저장할 최대 점 개수 (삼각형 1개당)

// 이동 모드
enum {
    MOVE_NONE = 0,  // 정지
    MOVE_BOUNCE,    // 1: 튕기기
    MOVE_ZIGZAG_H,  // 2: 좌우 지그재그
    MOVE_ZIGZAG_V,  // 3: 상하 뾰족 지그재그
    MOVE_SPIRAL     // 4: 원 스파이럴
};

typedef struct {
    float r, g, b;
} Color;

typedef struct {
    int active;        // 삼각형 존재 여부
    float posX, posY;  // 중심 위치 (NDC)
    float scale;       // 크기 배율
    Color color;       // 색상
    int isScalingUp;   // 우클릭 확대/축소 토글

    float speed;       // 이동 속도 (NDC/초) - 삼각형마다 다름
    float angle;       // 렌더링 회전각 (진행 방향을 바라보게)

    // 튕기기 / 상하 지그재그용 방향 벡터
    float dirX, dirY;

    // 좌우 지그재그용
    int zigH;          // 수평 진행 방향 (+1: 오른쪽, -1: 왼쪽)
    int zigV;          // 한 줄 내려갈/올라갈 방향 (-1: 아래, +1: 위)
    int zigPhase;      // 0: 수평 이동 중, 1: 수직으로 한 칸 이동 중
    float zigRemain;   // 수직 이동 남은 거리

    // 스파이럴용
    float cx, cy;      // 회전 중심
    float theta;       // 현재 각도
    float radius;      // 현재 반지름
    float omega;       // 각속도 (라디안/초)

    // 스파이럴 경로 (지나간 위치를 x,y,z 순서로 저장 → 그대로 VBO에 올림)
    float trail[TRAIL_MAX * 3];
    int trailCount;
} Triangle;

// 삼각형 목록 (좌클릭할 때마다 뒤에 추가)
#define MAX_TRI 5
Triangle tris[MAX_TRI];
int triCount = 0;

GLenum polygonMode = GL_FILL;
int moveMode = MOVE_NONE;

GLuint shaderProgramID;
GLuint triVAO, triVBO;
GLuint trailVAO, trailVBO;   // 경로 선 그리기용 (매 프레임 내용 갱신)

const float ZIG_STEP = 0.15f;      // 좌우 지그재그에서 한 줄 간격


GLuint makeShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint result;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
    if (!result) {
        char errorLog[512];
        glGetShaderInfoLog(shader, 512, NULL, errorLog);
        printf("셰이더 컴파일 실패:\n%s\n", errorLog);
    }
    return shader;
}

GLuint makeShaderProgram() {
    GLuint vertexShader = makeShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = makeShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint result;
    glGetProgramiv(program, GL_LINK_STATUS, &result);
    if (!result) {
        char errorLog[512];
        glGetProgramInfoLog(program, 512, NULL, errorLog);
        printf("셰이더 프로그램 링크 실패:\n%s\n", errorLog);
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

float randf(float a, float b) {
    return a + ((float)rand() / RAND_MAX) * (b - a);
}

int randSign() {
    return (rand() % 2) ? 1 : -1;
}

Color getRandomColor() {
    Color c;
    c.r = randf(0.0f, 1.0f);
    c.g = randf(0.0f, 1.0f);
    c.b = randf(0.0f, 1.0f);
    return c;
}

void initBuffers() {
    // 이등변삼각형 (꼭짓점이 +y 방향 = 기본적으로 위를 바라봄)
    float triVertices[] = {
         0.0f,   0.12f, 0.0f,  // 상단 (머리)
        -0.08f, -0.08f, 0.0f,  // 좌하단
         0.08f, -0.08f, 0.0f   // 우하단
    };

    glGenVertexArrays(1, &triVAO);
    glGenBuffers(1, &triVBO);
    glBindVertexArray(triVAO);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVertices), triVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 경로용 버퍼: 크기만 잡아두고(NULL) 그릴 때마다 glBufferSubData로 채운다
    glGenVertexArrays(1, &trailVAO);
    glGenBuffers(1, &trailVBO);
    glBindVertexArray(trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, trailVBO);
    glBufferData(GL_ARRAY_BUFFER, TRAIL_MAX * 3 * sizeof(float), NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}


// 삼각형이 회전해도 화면 밖으로 안 나가게 하는 여유 반경
float getRadius(const Triangle* t) {
    return 0.12f * t->scale;
}

// 경로에 현재 위치를 한 점 추가 (꽉 차면 가장 오래된 점을 밀어냄)
void addTrailPoint(Triangle* t) {
    if (t->trailCount > 0) {
        // 직전 점과 너무 가까우면 추가하지 않음 (점 낭비 방지)
        float* last = &t->trail[(t->trailCount - 1) * 3];
        float dx = t->posX - last[0];
        float dy = t->posY - last[1];
        if (dx * dx + dy * dy < 0.005f * 0.005f) return;
    }

    if (t->trailCount == TRAIL_MAX) {
        memmove(t->trail, t->trail + 3, (TRAIL_MAX - 1) * 3 * sizeof(float));
        t->trailCount--;
    }

    float* p = &t->trail[t->trailCount * 3];
    p[0] = t->posX;
    p[1] = t->posY;
    p[2] = 0.0f;
    t->trailCount++;
}

// 현재 모드에 맞게 삼각형의 이동 상태를 초기화
void initMotion(Triangle* t, int mode) {
    t->trailCount = 0;   // 모드가 바뀌거나 새로 생성되면 경로 초기화

    switch (mode) {
    case MOVE_BOUNCE: {
        // 축에 너무 평행하지 않은 랜덤 대각 방향
        float a = randf(0.2f, 1.37f) + (rand() % 4) * (PI_F / 2.0f);
        t->dirX = cosf(a);
        t->dirY = sinf(a);
        break;
    }
    case MOVE_ZIGZAG_H:
        t->zigH = randSign();
        t->zigV = -1;          // 기본은 아래로 내려가면서 지그재그
        t->zigPhase = 0;
        t->zigRemain = 0.0f;
        break;
    case MOVE_ZIGZAG_V: {
        // 수직 성분이 훨씬 큰 방향 → 뾰족한 V자 궤적
        float dx = randSign() * 0.3f;
        float dy = randSign() * 1.0f;
        float len = sqrtf(dx * dx + dy * dy);
        t->dirX = dx / len;
        t->dirY = dy / len;
        break;
    }
    case MOVE_SPIRAL:
        t->cx = t->posX;
        t->cy = t->posY;
        t->theta = randf(0.0f, 2.0f * PI_F);
        t->radius = 0.0f;
        t->omega = randSign() * randf(2.0f, 3.5f);
        addTrailPoint(t);   // 시작점(회전 중심)부터 경로 기록
        break;
    default:
        break;
    }
}

void setMoveMode(int mode) {
    moveMode = mode;
    for (int i = 0; i < triCount; ++i) {
        if (tris[i].active) initMotion(&tris[i], mode);
    }
}

// 1, 3번 공통: 벽에 닿으면 해당 축 방향을 반사
void updateBounce(Triangle* t, float dt) {
    float R = getRadius(t);
    t->posX += t->dirX * t->speed * dt;
    t->posY += t->dirY * t->speed * dt;

    if (t->posX > 1.0f - R) { t->posX = 1.0f - R; t->dirX = -fabsf(t->dirX); }
    if (t->posX < -1.0f + R) { t->posX = -1.0f + R; t->dirX = fabsf(t->dirX); }
    if (t->posY > 1.0f - R) { t->posY = 1.0f - R; t->dirY = -fabsf(t->dirY); }
    if (t->posY < -1.0f + R) { t->posY = -1.0f + R; t->dirY = fabsf(t->dirY); }
}

// 2번: 좌우로 가다가 벽에 닿으면 한 줄 내려가서(올라가서) 반대로
void updateZigzagH(Triangle* t, float dt) {
    float R = getRadius(t);
    float d = t->speed * dt;

    if (t->zigPhase == 0) {
        t->posX += t->zigH * d;
        if (t->posX > 1.0f - R || t->posX < -1.0f + R) {
            t->posX = (t->posX > 0.0f) ? 1.0f - R : -1.0f + R;
            t->zigPhase = 1;          // 수직 이동 단계로
            t->zigRemain = ZIG_STEP;
        }
    }
    else {
        if (d > t->zigRemain) d = t->zigRemain;
        t->posY += t->zigV * d;
        t->zigRemain -= d;

        // 위/아래 끝에 닿으면 줄 이동 방향 자체를 뒤집음
        if (t->posY < -1.0f + R) { t->posY = -1.0f + R; t->zigV = 1; }
        if (t->posY > 1.0f - R) { t->posY = 1.0f - R; t->zigV = -1; }

        if (t->zigRemain <= 0.0f) {
            t->zigPhase = 0;
            t->zigH = -t->zigH;       // 수평 방향 반전
        }
    }
}

// 4번: 중심을 돌면서 반지름이 점점 커짐
//      벽에 닿으면 원래 위치(회전 중심)로 돌아가서 처음부터 다시 스파이럴
//      반환값: 1이면 이번 프레임에 원래 위치로 되돌아감
int updateSpiral(Triangle* t, float dt) {
    float R = getRadius(t);

    t->theta += t->omega * dt;
    t->radius += 0.04f * dt;

    float nx = t->cx + t->radius * cosf(t->theta);
    float ny = t->cy + t->radius * sinf(t->theta);

    int hitEdge = (nx > 1.0f - R || nx < -1.0f + R || ny > 1.0f - R || ny < -1.0f + R);
    if (hitEdge) {
        // 원래 위치로 복귀 + 경로도 처음부터 다시 그림
        t->radius = 0.0f;
        t->posX = t->cx;
        t->posY = t->cy;
        t->trailCount = 0;
        return 1;
    }

    t->posX = nx;
    t->posY = ny;
    return 0;
}

void updateTriangles(float dt) {
    if (moveMode == MOVE_NONE) return;

    for (int i = 0; i < triCount; ++i) {
        Triangle* t = &tris[i];
        if (!t->active) continue;

        float oldX = t->posX, oldY = t->posY;

        switch (moveMode) {
        case MOVE_BOUNCE:
        case MOVE_ZIGZAG_V: updateBounce(t, dt);  break;
        case MOVE_ZIGZAG_H: updateZigzagH(t, dt); break;
        case MOVE_SPIRAL:
            if (updateSpiral(t, dt)) {
                // 원래 위치로 순간이동한 프레임은 방향 계산을 건너뜀
                // (안 그러면 벽->중심 방향으로 한 프레임 휙 돌아감)
                addTrailPoint(t);
                continue;
            }
            addTrailPoint(t);
            break;
        }

        // 실제로 움직인 방향으로 머리(꼭짓점)를 돌린다
        // 모델의 머리가 +y이므로 -90도 보정
        float dx = t->posX - oldX;
        float dy = t->posY - oldY;
        if (fabsf(dx) > 0 || fabsf(dy) > 0) {
            t->angle = atan2f(dy, dx) - PI_F / 2.0f;
        }
    }
}


void setTriangle(Triangle* t, float x, float y, float scale) {
    t->active = 1;
    t->posX = x;
    t->posY = y;
    t->scale = scale;
    t->color = getRandomColor();
    t->isScalingUp = 1;
    t->speed = randf(0.25f, 0.7f);   // 삼각형마다 다른 속도
    t->angle = 0.0f;
    initMotion(t, moveMode);
}

void initTriangles() {
    triCount = 0;   // 빈 화면으로 시작 (좌클릭으로만 생성)
}

// 클릭 위치에 삼각형 추가 (최대 MAX_TRI개, 꽉 차면 더 이상 안 만들어짐)
void addTriangle(float x, float y) {
    if (triCount >= MAX_TRI) return;
    setTriangle(&tris[triCount], x, y, randf(0.6f, 1.6f));
    triCount++;
}

// 클릭 위치에서 가장 가까운 삼각형 번호 (없으면 -1)
int findNearestTriangle(float x, float y) {
    int best = -1;
    float bestDist = 1e9f;
    for (int i = 0; i < triCount; ++i) {
        float dx = tris[i].posX - x;
        float dy = tris[i].posY - y;
        float d = dx * dx + dy * dy;
        if (d < bestDist) { bestDist = d; best = i; }
    }
    return best;
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
    case GLFW_KEY_1: setMoveMode(MOVE_BOUNCE);   break;
    case GLFW_KEY_2: setMoveMode(MOVE_ZIGZAG_H); break;
    case GLFW_KEY_3: setMoveMode(MOVE_ZIGZAG_V); break;
    case GLFW_KEY_4: setMoveMode(MOVE_SPIRAL);   break;
    case GLFW_KEY_0: setMoveMode(MOVE_NONE);     break;  // 정지 (추가 기능)

    case GLFW_KEY_A: polygonMode = GL_FILL; break;
    case GLFW_KEY_B: polygonMode = GL_LINE; break;
    case GLFW_KEY_C:
        moveMode = MOVE_NONE;
        initTriangles();
        break;
    case GLFW_KEY_Q:
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (action != GLFW_PRESS) return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);

    // 커서 좌표는 윈도우 좌표계라서 윈도우 크기로 나눈다
    int width, height;
    glfwGetWindowSize(window, &width, &height);

    float glX = (float)((xpos / width) * 2.0 - 1.0);
    float glY = (float)(1.0 - (ypos / height) * 2.0);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        // 클릭 위치에 새 삼각형 추가 (현재 이동 모드로 바로 움직임)
        addTriangle(glX, glY);
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        // 클릭 위치에서 가장 가까운 삼각형을 확대/축소
        int idx = findNearestTriangle(glX, glY);
        if (idx >= 0) {
            Triangle* t = &tris[idx];
            if (t->isScalingUp) {
                t->scale *= 1.4f;
                if (t->scale > 2.2f) { t->scale = 2.2f; t->isScalingUp = 0; }
            }
            else {
                t->scale *= 0.7f;
                if (t->scale < 0.4f) { t->scale = 0.4f; t->isScalingUp = 1; }
            }
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}


void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    GLint transLoc = glGetUniformLocation(shaderProgramID, "uTranslation");
    GLint scaleLoc = glGetUniformLocation(shaderProgramID, "uScale");
    GLint angleLoc = glGetUniformLocation(shaderProgramID, "uAngle");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "uColor");

    // 1. 스파이럴 경로 (4번 모드일 때만, 삼각형 밑에 깔리도록 먼저 그림)
    //    경로 점은 이미 NDC 좌표라서 이동/크기/회전 uniform은 기본값으로 둔다
    if (moveMode == MOVE_SPIRAL) {
        glUniform2f(transLoc, 0.0f, 0.0f);
        glUniform1f(scaleLoc, 1.0f);
        glUniform1f(angleLoc, 0.0f);

        glBindVertexArray(trailVAO);
        glBindBuffer(GL_ARRAY_BUFFER, trailVBO);

        for (int i = 0; i < triCount; ++i) {
            const Triangle* t = &tris[i];
            if (!t->active || t->trailCount < 2) continue;

            glBufferSubData(GL_ARRAY_BUFFER, 0, t->trailCount * 3 * sizeof(float), t->trail);
            glUniform4f(colorLoc, t->color.r, t->color.g, t->color.b, 1.0f);
            glDrawArrays(GL_LINE_STRIP, 0, t->trailCount);
        }
    }

    // 2. 삼각형들 (같은 VAO를 uniform만 바꿔가며 triCount번 그림)
    glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
    glBindVertexArray(triVAO);

    for (int i = 0; i < triCount; ++i) {
        const Triangle* t = &tris[i];
        if (!t->active) continue;

        glUniform2f(transLoc, t->posX, t->posY);
        glUniform1f(scaleLoc, t->scale);
        glUniform1f(angleLoc, t->angle);
        glUniform4f(colorLoc, t->color.r, t->color.g, t->color.b, 1.0f);

        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    glBindVertexArray(0);
}


int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "GLSL Practice 9 - Moving Triangles", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // V-Sync

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    shaderProgramID = makeShaderProgram();
    initBuffers();
    initTriangles();

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        // 프레임 간 시간(dt)으로 이동 → 컴퓨터 속도와 무관하게 일정한 속도
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;
        if (dt > 0.05f) dt = 0.05f;  // 창 드래그 등으로 멈췄을 때 순간이동 방지

        updateTriangles(dt);
        DrawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}