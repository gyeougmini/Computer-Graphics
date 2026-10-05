#include <GL/glew.h>
#include <GL/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <vector>


const char* vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 vPos;\n"
"uniform vec2 uTranslation;\n"
"uniform vec2 uScale;\n"
"void main()\n"
"{\n"
"    vec2 p = vPos.xy * uScale;\n"
"    gl_Position = vec4(p + uTranslation, vPos.z, 1.0);\n"
"}\n";

const char* fragmentShaderSource =
"#version 330 core\n"
"out vec4 FragColor;\n"
"uniform vec4 uColor;\n"
"void main()\n"
"{\n"
"    FragColor = uColor;\n"
"}\n";


typedef struct {
    float r, g, b;
} Color;

// 사각형 상태 구조체
typedef struct {
    float x, y;          // 위치 (NDC)
    float width, height; // 크기
    float speed;         // 이동 속도
    int dir;             // 1: 위, -1: 아래
    Color color;

    // 이동/애니메이션 상태
    int isMoving;        // 1: 상하 이동 중, 0: 멈춤/오른쪽으로 쌓임
    int isStacking;      // 1: 오른쪽 탑 위치로 애니메이션 이동 중
    float targetX;       // 애니메이션 목표 X
    float targetY;       // 애니메이션 목표 Y
} Box;

// 상하 이동 사각형 2개
Box movingBoxes[2];

// 오른쪽 탑에 쌓인 사각형들
std::vector<Box> stackedBoxes;

GLuint shaderProgramID;
GLuint rectVAO, rectVBO;
GLuint lineVAO, lineVBO;

// 판정 및 가이드 박스 영역 (NDC 좌표)
const float ZONE1_X = -0.6f;
const float ZONE2_X = -0.1f;
const float TOWER_X = 0.6f;

const float MATCH_Y_MIN = -0.15f;
const float MATCH_Y_MAX = 0.15f;


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

Color getRandomColor() {
    Color c;
    c.r = randf(0.2f, 0.9f);
    c.g = randf(0.2f, 0.9f);
    c.b = randf(0.2f, 0.9f);
    return c;
}

void initBuffers() {
    // 중심이 (0,0)인 기본 사각형 (크기 1.0 x 1.0)
    float rectVertices[] = {
        -0.5f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,

        -0.5f,  0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.5f,  0.5f, 0.0f
    };

    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 단위 라인 (외곽선/가이드 라인 그리기용)
    float lineVertices[] = {
        -0.5f, -0.5f, 0.0f,  0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,  0.5f,  0.5f, 0.0f,
         0.5f,  0.5f, 0.0f, -0.5f,  0.5f, 0.0f,
        -0.5f,  0.5f, 0.0f, -0.5f, -0.5f, 0.0f
    };

    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}


void createMovingBox(int index) {
    movingBoxes[index].x = (index == 0) ? ZONE1_X : ZONE2_X;
    movingBoxes[index].y = randf(-0.7f, 0.7f);
    movingBoxes[index].width = 0.25f;
    movingBoxes[index].height = 0.12f;
    movingBoxes[index].speed = randf(0.6f, 1.2f);
    movingBoxes[index].dir = (rand() % 2 == 0) ? 1 : -1;
    movingBoxes[index].color = getRandomColor();
    movingBoxes[index].isMoving = 1;
    movingBoxes[index].isStacking = 0;
}

void initGame() {
    stackedBoxes.clear();
    createMovingBox(0);
    createMovingBox(1);
}

void updateGame(float dt) {
    // 1. 상하 이동 사각형 업데이트
    for (int i = 0; i < 2; ++i) {
        if (movingBoxes[i].isMoving) {
            movingBoxes[i].y += movingBoxes[i].dir * movingBoxes[i].speed * dt;

            // 상하 한계 영역 튕기기
            if (movingBoxes[i].y > 0.8f) {
                movingBoxes[i].y = 0.8f;
                movingBoxes[i].dir = -1;
            }
            if (movingBoxes[i].y < -0.8f) {
                movingBoxes[i].y = -0.8f;
                movingBoxes[i].dir = 1;
            }
        }
    }

    // 2. 우측 이동 애니메이션 업데이트 (쌓이는 사각형들)
    for (size_t i = 0; i < stackedBoxes.size(); ++i) {
        if (stackedBoxes[i].isStacking) {
            float speed = 2.0f * dt;

            // X축 오른쪽으로 이동
            if (stackedBoxes[i].x < stackedBoxes[i].targetX) {
                stackedBoxes[i].x += speed;
                if (stackedBoxes[i].x >= stackedBoxes[i].targetX) {
                    stackedBoxes[i].x = stackedBoxes[i].targetX;
                }
            }
            // Y축 쌓일 위치로 이동
            if (fabsf(stackedBoxes[i].y - stackedBoxes[i].targetY) > 0.01f) {
                stackedBoxes[i].y += (stackedBoxes[i].targetY - stackedBoxes[i].y) * 5.0f * dt;
            }
            else {
                stackedBoxes[i].y = stackedBoxes[i].targetY;
            }

            if (stackedBoxes[i].x == stackedBoxes[i].targetX && stackedBoxes[i].y == stackedBoxes[i].targetY) {
                stackedBoxes[i].isStacking = 0;
            }
        }
    }
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
    case GLFW_KEY_ENTER: {
        // 두 사각형이 모두 일치하는 지정 영역(MATCH_Y) 안에 들어왔는지 검사
        int inZone0 = (movingBoxes[0].y >= MATCH_Y_MIN && movingBoxes[0].y <= MATCH_Y_MAX);
        int inZone1 = (movingBoxes[1].y >= MATCH_Y_MIN && movingBoxes[1].y <= MATCH_Y_MAX);

        if (inZone0 && inZone1 && movingBoxes[0].isMoving && movingBoxes[1].isMoving) {
            // 위아래 이동 멈춤
            movingBoxes[0].isMoving = 0;
            movingBoxes[1].isMoving = 0;

            // 오른쪽 탑 위치 계산 및 쌓기 애니메이션 등록
            for (int i = 0; i < 2; ++i) {
                Box b = movingBoxes[i];
                b.isStacking = 1;
                b.targetX = TOWER_X;

                // 탑 쌓기 높이 계산
                float stackY = -0.8f + (stackedBoxes.size() * (b.height + 0.02f));
                b.targetY = stackY;

                stackedBoxes.push_back(b);
            }

            // 새로운 사각형 2개 다시 생성
            createMovingBox(0);
            createMovingBox(1);
            printf(">> 맞추기 성공! 우측으로 사각형이 쌓입니다.\n");
        }
        else {
            printf(">> 맞추기 실패! 두 사각형이 점선 영역에 같이 있어야 합니다.\n");
        }
        break;
    }
    case GLFW_KEY_R:
        initGame();
        printf(">> 리셋 되었습니다.\n");
        break;
    case GLFW_KEY_Q:
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}


void drawRect(float x, float y, float w, float h, Color c, GLenum mode) {
    GLint transLoc = glGetUniformLocation(shaderProgramID, "uTranslation");
    GLint scaleLoc = glGetUniformLocation(shaderProgramID, "uScale");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "uColor");

    glUniform2f(transLoc, x, y);
    glUniform2f(scaleLoc, w, h);
    glUniform4f(colorLoc, c.r, c.g, c.b, 1.0f);

    if (mode == GL_FILL) {
        glBindVertexArray(rectVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    else {
        glBindVertexArray(lineVAO);
        glDrawArrays(GL_LINES, 0, 8);
    }
}

void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    // 1. 공간 가이드 영역 라인 그리기 (셰이더 이용)
    Color guideColor = { 0.5f, 0.5f, 0.5f };
    drawRect(ZONE1_X, 0.0f, 0.35f, 1.7f, guideColor, GL_LINE);
    drawRect(ZONE2_X, 0.0f, 0.35f, 1.7f, guideColor, GL_LINE);

    // 2. 두 사각형이 맞닿는 중앙 판정 영역 (점선/가이드 박스)
    Color targetZoneColor = { 0.7f, 0.7f, 0.7f };
    drawRect((ZONE1_X + ZONE2_X) / 2.0f, 0.0f, 0.85f, MATCH_Y_MAX - MATCH_Y_MIN, targetZoneColor, GL_LINE);

    // 3. 상하 이동 사각형 2개 그리기
    for (int i = 0; i < 2; ++i) {
        if (movingBoxes[i].isMoving) {
            drawRect(movingBoxes[i].x, movingBoxes[i].y, movingBoxes[i].width, movingBoxes[i].height, movingBoxes[i].color, GL_FILL);
        }
    }

    // 4. 오른쪽으로 쌓이거나 쌓인 사각형 탑 그리기
    for (size_t i = 0; i < stackedBoxes.size(); ++i) {
        drawRect(stackedBoxes[i].x, stackedBoxes[i].y, stackedBoxes[i].width, stackedBoxes[i].height, stackedBoxes[i].color, GL_FILL);
    }

    glBindVertexArray(0);
}


int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "GLSL Practice 12 - Matching Rectangles", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    shaderProgramID = makeShaderProgram();
    initBuffers();
    initGame();

    glfwSetKeyCallback(window, key_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window)) {
        double now = glfwGetTime();
        float dt = (float)(now - lastTime);
        lastTime = now;
        if (dt > 0.05f) dt = 0.05f;

        updateGame(dt);
        DrawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}