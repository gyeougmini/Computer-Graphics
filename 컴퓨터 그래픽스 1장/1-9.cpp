#include <GL/glew.h>
#include <GL/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define MAX_TRIANGLES 5
#define PI 3.14159265358979323846f

// ==========================================
// 1. GLSL 셰이더 소스코드 (C 문자열)
// ==========================================

const char* vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 vPos;\n"
"uniform vec2 uTranslation;\n"
"uniform float uScale;\n"
"uniform float uAngle;\n"
"void main()\n"
"{\n"
"    // Z축 기준 회전 변환\n"
"    float c = cos(uAngle);\n"
"    float s = sin(uAngle);\n"
"    vec2 rotatedPos = vec2(vPos.x * c - vPos.y * s, vPos.x * s + vPos.y * c);\n"
"    \n"
"    // 크기 조절 및 위치 이동\n"
"    vec2 finalPos = rotatedPos * uScale + uTranslation;\n"
"    gl_Position = vec4(finalPos, vPos.z, 1.0);\n"
"}\n";

const char* fragmentShaderSource =
"#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec4 uColor;\n"
"void main()\n"
"{\n"
"    FragColor = uColor;\n"
"}\n";

// ==========================================
// 2. 구조체 및 전역 변수
// ==========================================

typedef enum {
    MOVE_NONE = 0,
    MOVE_BOUNCE = 1,    // 1: 튕기기
    MOVE_ZIGZAG_H = 2,  // 2: 가로 지그재그 (벽 충돌 시 회전)
    MOVE_ZIGZAG_V = 3,  // 3: 위아래 지그재그 (천장/바닥 충돌 시 회전)
    MOVE_SPIRAL = 4     // 4: 원 스파이럴 (자전 동기화)
} MoveMode;

typedef struct {
    float r, g, b;
} Color;

typedef struct {
    int active;
    float posX, posY;      // 위치
    float scale;           // 크기
    float angle;           // 회전각 (라디안)
    Color color;           // 색상
    int isScalingUp;       // 확대/축소 상태

    // 이동 속도 및 나선 변수
    float vx, vy;
    float initX, initY;
    float spiralRadius;
    float spiralAngle;
} Triangle;

Triangle triangles[MAX_TRIANGLES];
int triangleCount = 0;

MoveMode currentMoveMode = MOVE_NONE;
GLenum polygonMode = GL_FILL;

GLuint shaderProgramID;
GLuint triVAO, triVBO;

double lastTime = 0.0;

// ==========================================
// 3. 셰이더 및 버퍼 관리
// ==========================================

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

Color getRandomColor() {
    Color c;
    c.r = (float)rand() / RAND_MAX;
    c.g = (float)rand() / RAND_MAX;
    c.b = (float)rand() / RAND_MAX;
    return c;
}

void initBuffers() {
    // 회전 시 왜곡이 없는 정교한 표준 이등변삼각형 정점
    float triVertices[] = {
         0.00f,  0.10f, 0.0f,  // 상단 꼭지점
        -0.08f, -0.06f, 0.0f,  // 좌하단
         0.08f, -0.06f, 0.0f   // 우하단
    };

    glGenVertexArrays(1, &triVAO);
    glGenBuffers(1, &triVBO);
    glBindVertexArray(triVAO);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVertices), triVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// 클릭 시 항상 위쪽을 바라보는 정방향 이등변삼각형으로 생성
void addTriangle(float x, float y) {
    if (triangleCount >= MAX_TRIANGLES) {
        printf("최대 삼각형 개수(%d개)에 도달했습니다.\n", MAX_TRIANGLES);
        return;
    }

    Triangle* t = &triangles[triangleCount];
    t->active = 1;
    t->posX = x;
    t->posY = y;
    t->initX = x;
    t->initY = y;
    t->scale = 0.8f + ((float)rand() / RAND_MAX) * 0.6f;

    // 처음에 생성될 때 위쪽을 바라봄 (0도)
    t->angle = 0.0f;

    t->color = getRandomColor();
    t->isScalingUp = 1;

    float spdX = 0.8f + ((float)rand() / RAND_MAX) * 0.4f;
    float spdY = 0.8f + ((float)rand() / RAND_MAX) * 0.4f;
    t->vx = (rand() % 2 == 0) ? spdX : -spdX;
    t->vy = (rand() % 2 == 0) ? spdY : -spdY;

    t->spiralRadius = 0.02f;
    t->spiralAngle = 0.0f;

    triangleCount++;
}

// ==========================================
// 4. 이동 및 회전 업데이트 (실습 9)
// ==========================================

void updateTriangles(float deltaTime) {
    if (currentMoveMode == MOVE_NONE) return;

    const float boundX = 0.88f;
    const float boundY = 0.88f;
    const float stepY = 0.15f;

    for (int i = 0; i < triangleCount; ++i) {
        if (!triangles[i].active) continue;

        Triangle* t = &triangles[i];

        switch (currentMoveMode) {
        case MOVE_BOUNCE: // 1: 사선 튕기기
            t->posX += t->vx * deltaTime;
            t->posY += t->vy * deltaTime;

            if (t->posX > boundX) { t->posX = boundX; t->vx = -t->vx; }
            else if (t->posX < -boundX) { t->posX = -boundX; t->vx = -t->vx; }

            if (t->posY > boundY) { t->posY = boundY; t->vy = -t->vy; }
            else if (t->posY < -boundY) { t->posY = -boundY; t->vy = -t->vy; }
            break;

        case MOVE_ZIGZAG_H: // 2: 가로 직선 이동 -> 벽 충돌 시 회전하여 밑변이 닿음
            t->posX += t->vx * deltaTime;

            if (t->posX > boundX) {
                t->posX = boundX;
                t->vx = -t->vx;
                t->posY -= stepY;
                t->angle = -PI / 2.0f; // -90도 회전 (밑변이 오른쪽 벽에 닿음)
            }
            else if (t->posX < -boundX) {
                t->posX = -boundX;
                t->vx = -t->vx;
                t->posY -= stepY;
                t->angle = PI / 2.0f; // 90도 회전 (밑변이 왼쪽 벽에 닿음)
            }

            if (t->posY < -boundY) {
                t->posY = boundY;
            }
            break;

        case MOVE_ZIGZAG_V: // 3: 위아래 지그재그 -> 천장/바닥 충돌 시 회전하여 밑변이 닿음
            t->posX += t->vx * 0.4f * deltaTime;
            t->posY += t->vy * 1.5f * deltaTime;

            if (t->posX > boundX) { t->posX = boundX; t->vx = -t->vx; }
            else if (t->posX < -boundX) { t->posX = -boundX; t->vx = -t->vx; }

            if (t->posY > boundY) {
                t->posY = boundY;
                t->vy = -t->vy;
                t->angle = PI; // 180도 회전 (밑변이 천장에 닿음)
            }
            else if (t->posY < -boundY) {
                t->posY = -boundY;
                t->vy = -t->vy;
                t->angle = 0.0f; // 0도 회전 (밑변이 바닥에 닿음)
            }
            break;

        case MOVE_SPIRAL: // 4: 원 스파이럴 (360도 자전 회전)
            t->spiralAngle += 3.0f * deltaTime;
            t->spiralRadius += 0.12f * deltaTime;

            t->posX = t->initX + t->spiralRadius * cosf(t->spiralAngle);
            t->posY = t->initY + t->spiralRadius * sinf(t->spiralAngle);

            t->angle = t->spiralAngle; // 스파이럴 자전 연동

            if (t->posX > boundX || t->posX < -boundX || t->posY > boundY || t->posY < -boundY) {
                t->spiralRadius = 0.02f;
            }
            break;

        default:
            break;
        }
    }
}

// ==========================================
// 5. 콜백 함수 (토글 키보드 & 마우스 클릭)
// ==========================================

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
    case GLFW_KEY_1:
        currentMoveMode = (currentMoveMode == MOVE_BOUNCE) ? MOVE_NONE : MOVE_BOUNCE;
        break;
    case GLFW_KEY_2:
        currentMoveMode = (currentMoveMode == MOVE_ZIGZAG_H) ? MOVE_NONE : MOVE_ZIGZAG_H;
        break;
    case GLFW_KEY_3:
        currentMoveMode = (currentMoveMode == MOVE_ZIGZAG_V) ? MOVE_NONE : MOVE_ZIGZAG_V;
        break;
    case GLFW_KEY_4:
        currentMoveMode = (currentMoveMode == MOVE_SPIRAL) ? MOVE_NONE : MOVE_SPIRAL;
        break;

    case GLFW_KEY_A: polygonMode = GL_FILL; break;
    case GLFW_KEY_B: polygonMode = GL_LINE; break;
    case GLFW_KEY_C:
        triangleCount = 0;
        currentMoveMode = MOVE_NONE;
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

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    float glX = (float)((xpos / width) * 2.0 - 1.0);
    float glY = (float)(1.0 - (ypos / height) * 2.0);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        addTriangle(glX, glY);
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        const float clickRange = 0.15f;
        for (int i = triangleCount - 1; i >= 0; --i) {
            float dx = glX - triangles[i].posX;
            float dy = glY - triangles[i].posY;
            if (sqrtf(dx * dx + dy * dy) <= clickRange) {
                if (triangles[i].isScalingUp) {
                    triangles[i].scale *= 1.3f;
                    if (triangles[i].scale > 2.2f) {
                        triangles[i].scale = 2.2f;
                        triangles[i].isScalingUp = 0;
                    }
                }
                else {
                    triangles[i].scale *= 0.7f;
                    if (triangles[i].scale < 0.4f) {
                        triangles[i].scale = 0.4f;
                        triangles[i].isScalingUp = 1;
                    }
                }
                break;
            }
        }
    }
}

// ==========================================
// 6. 렌더링 함수
// ==========================================

void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    GLint transLoc = glGetUniformLocation(shaderProgramID, "uTranslation");
    GLint scaleLoc = glGetUniformLocation(shaderProgramID, "uScale");
    GLint angleLoc = glGetUniformLocation(shaderProgramID, "uAngle");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "uColor");

    glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
    glBindVertexArray(triVAO);

    for (int i = 0; i < triangleCount; ++i) {
        if (triangles[i].active) {
            glUniform2f(transLoc, triangles[i].posX, triangles[i].posY);
            glUniform1f(scaleLoc, triangles[i].scale);
            glUniform1f(angleLoc, triangles[i].angle);
            glUniform4f(colorLoc, triangles[i].color.r, triangles[i].color.g, triangles[i].color.b, 1.0f);

            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }

    glBindVertexArray(0);
}

// ==========================================
// 7. 메인 함수
// ==========================================

int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "GLSL Practice 9 - Perfect Triangle", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    shaderProgramID = makeShaderProgram();
    initBuffers();

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    glViewport(0, 0, 800, 600);
    lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;

        updateTriangles(deltaTime);

        DrawScene();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}