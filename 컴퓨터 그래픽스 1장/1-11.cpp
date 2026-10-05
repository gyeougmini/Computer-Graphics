#include <GL/glew.h>
#include <GL/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>


// 버텍스 셰이더: 평면 이동(uTranslation) 및 크기 조절(uScale)
const char* vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 vPos;\n"
"uniform vec2 uTranslation;\n"
"uniform float uScale;\n"
"void main()\n"
"{\n"
"    vec2 p = vPos.xy * uScale;\n"
"    gl_Position = vec4(p + uTranslation, vPos.z, 1.0);\n"
"}\n";

// 프래그먼트 셰이더: 단일 색상(uColor) 출력
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

enum ShapeType {
    SHAPE_SQUARE = 0,    // 사각형
    SHAPE_TRIANGLE,      // 정삼각형
    SHAPE_INVERTED_TRI   // 역삼각형
};

typedef struct {
    int gridX, gridY;     // 그리드 좌표
    ShapeType shape;      // 현재 모양
    Color color;          // 색상
    float scale;          // 크기 (칸 내부 크기 배율)
} Entity;


int boardWidth = 20;      // N (입력받음)
int boardHeight = 20;     // M (입력받음)

GLuint shaderProgramID;
GLuint squareVAO, squareVBO;
GLuint triVAO, triVBO;
GLuint invTriVAO, invTriVBO;
GLuint gridVAO, gridVBO;

Entity player;
#define MAX_OBSTACLES 200
Entity obstacles[MAX_OBSTACLES];
int obstacleCount = 0;

// 주인공 지그재그 이동 및 속도 관련
int isMoving = 0;         // 's' 키 입력 시 토글
int playerDirX = 1;       // +1: 오른쪽, -1: 왼쪽
float moveTimer = 0.0f;
float moveInterval = 0.15f; // 한 칸 이동하는 데 걸리는 시간(초) - 낮을수록 빨라짐

// 충돌 효과 변수
float collisionEffectTimer = 0.0f; // 충돌 효과 잔여 시간 (초)
Color collisionEffectColor = { 1.0f, 0.0f, 0.0f }; // 효과 색상


float randf(float a, float b) {
    return a + ((float)rand() / RAND_MAX) * (b - a);
}

Color getRandomColor() {
    Color c;
    c.r = randf(0.1f, 1.0f);
    c.g = randf(0.1f, 1.0f);
    c.b = randf(0.1f, 1.0f);
    return c;
}

// 그리드 좌표 (gx, gy)를 NDC (-1 ~ +1) 셀의 중심 좌표로 변환
void gridToNDC(int gx, int gy, float* outX, float* outY) {
    float cellW = 2.0f / boardWidth;
    float cellH = 2.0f / boardHeight;

    *outX = -1.0f + (gx + 0.5f) * cellW;
    *outY = 1.0f - (gy + 0.5f) * cellH; // Y축은 위쪽이 +1
}

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

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}


void initBuffers() {
    float cellW = 2.0f / boardWidth;
    float cellH = 2.0f / boardHeight;
    float hW = cellW * 0.5f;
    float hH = cellH * 0.5f;

    // 1. 사각형 (Square)
    float sqVerts[] = {
        -hW,  hH, 0.0f,   -hW, -hH, 0.0f,    hW, -hH, 0.0f,
        -hW,  hH, 0.0f,    hW, -hH, 0.0f,    hW,  hH, 0.0f
    };
    glGenVertexArrays(1, &squareVAO);
    glGenBuffers(1, &squareVBO);
    glBindVertexArray(squareVAO);
    glBindBuffer(GL_ARRAY_BUFFER, squareVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(sqVerts), sqVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 2. 정삼각형 (Triangle)
    float triVerts[] = {
         0.0f,  hH, 0.0f,
        -hW,   -hH, 0.0f,
         hW,   -hH, 0.0f
    };
    glGenVertexArrays(1, &triVAO);
    glGenBuffers(1, &triVBO);
    glBindVertexArray(triVAO);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVerts), triVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 3. 역삼각형 (Inverted Triangle)
    float invTriVerts[] = {
        -hW,    hH, 0.0f,
         hW,    hH, 0.0f,
         0.0f, -hH, 0.0f
    };
    glGenVertexArrays(1, &invTriVAO);
    glGenBuffers(1, &invTriVBO);
    glBindVertexArray(invTriVAO);
    glBindBuffer(GL_ARRAY_BUFFER, invTriVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(invTriVerts), invTriVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 4. 보드판 격자선 (Grid Lines)
    int linePointCount = ((boardWidth + 1) + (boardHeight + 1)) * 2;
    float* gridLines = (float*)malloc(linePointCount * 3 * sizeof(float));
    int idx = 0;

    for (int i = 0; i <= boardWidth; ++i) {
        float x = -1.0f + i * cellW;
        gridLines[idx++] = x;     gridLines[idx++] = 1.0f;  gridLines[idx++] = 0.0f;
        gridLines[idx++] = x;     gridLines[idx++] = -1.0f; gridLines[idx++] = 0.0f;
    }
    for (int j = 0; j <= boardHeight; ++j) {
        float y = 1.0f - j * cellH;
        gridLines[idx++] = -1.0f; gridLines[idx++] = y;     gridLines[idx++] = 0.0f;
        gridLines[idx++] = 1.0f;  gridLines[idx++] = y;     gridLines[idx++] = 0.0f;
    }

    glGenVertexArrays(1, &gridVAO);
    glGenBuffers(1, &gridVBO);
    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, linePointCount * 3 * sizeof(float), gridLines, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    free(gridLines);
    glBindVertexArray(0);
}

void initBoardAndEntities() {
    player.gridX = 0;
    player.gridY = 0;
    player.shape = SHAPE_SQUARE;
    player.color = getRandomColor();
    player.scale = 0.85f;

    playerDirX = 1;
    isMoving = 0;
    moveTimer = 0.0f;
    moveInterval = 0.15f; // 초기 속도 기본값 설정
    collisionEffectTimer = 0.0f;

    int totalCells = boardWidth * boardHeight;
    obstacleCount = totalCells / 4;
    if (obstacleCount > MAX_OBSTACLES) obstacleCount = MAX_OBSTACLES;

    int occupied[100][100] = { 0 };
    occupied[0][0] = 1;

    for (int i = 0; i < obstacleCount; ++i) {
        int gx, gy;
        do {
            gx = rand() % boardWidth;
            gy = rand() % boardHeight;
        } while (occupied[gy][gx]);

        occupied[gy][gx] = 1;

        obstacles[i].gridX = gx;
        obstacles[i].gridY = gy;
        obstacles[i].shape = (ShapeType)(rand() % 3);
        obstacles[i].color = getRandomColor();
        obstacles[i].scale = randf(0.5f, 0.9f);
    }
}


void checkCollision() {
    for (int i = 0; i < obstacleCount; ++i) {
        if (obstacles[i].gridX == player.gridX && obstacles[i].gridY == player.gridY) {
            ShapeType tempShape = player.shape;
            Color tempColor = player.color;

            player.shape = obstacles[i].shape;
            player.color = obstacles[i].color;

            obstacles[i].shape = tempShape;
            obstacles[i].color = tempColor;

            collisionEffectTimer = 0.4f;
            collisionEffectColor = getRandomColor();
            break;
        }
    }
}

void updatePlayer(float dt) {
    if (collisionEffectTimer > 0.0f) {
        collisionEffectTimer -= dt;
    }

    if (!isMoving) return;

    moveTimer += dt;
    if (moveTimer >= moveInterval) {
        moveTimer = 0.0f;

        int nextX = player.gridX + playerDirX;

        if (nextX >= 0 && nextX < boardWidth) {
            player.gridX = nextX;
        }
        else {
            if (player.gridY + 1 < boardHeight) {
                player.gridY += 1;
                playerDirX = -playerDirX;
            }
            else {
                isMoving = 0;
                printf("[알림] 주인공이 보드판의 마지막 칸에 도달했습니다!\n");
            }
        }

        checkCollision();
    }
}


void drawCellBackground(int gx, int gy, Color col) {
    float ndcX, ndcY;
    gridToNDC(gx, gy, &ndcX, &ndcY);

    GLint transLoc = glGetUniformLocation(shaderProgramID, "uTranslation");
    GLint scaleLoc = glGetUniformLocation(shaderProgramID, "uScale");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "uColor");

    glUniform2f(transLoc, ndcX, ndcY);
    glUniform1f(scaleLoc, 1.0f);
    glUniform4f(colorLoc, col.r, col.g, col.b, 1.0f);

    glBindVertexArray(squareVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void drawEntity(const Entity* ent) {
    float ndcX, ndcY;
    gridToNDC(ent->gridX, ent->gridY, &ndcX, &ndcY);

    GLint transLoc = glGetUniformLocation(shaderProgramID, "uTranslation");
    GLint scaleLoc = glGetUniformLocation(shaderProgramID, "uScale");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "uColor");

    glUniform2f(transLoc, ndcX, ndcY);
    glUniform1f(scaleLoc, ent->scale);
    glUniform4f(colorLoc, ent->color.r, ent->color.g, ent->color.b, 1.0f);

    if (ent->shape == SHAPE_SQUARE) {
        glBindVertexArray(squareVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    else if (ent->shape == SHAPE_TRIANGLE) {
        glBindVertexArray(triVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    else if (ent->shape == SHAPE_INVERTED_TRI) {
        glBindVertexArray(invTriVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
}

void drawScene() {
    glClearColor(0.95f, 0.95f, 0.95f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    GLint transLoc = glGetUniformLocation(shaderProgramID, "uTranslation");
    GLint scaleLoc = glGetUniformLocation(shaderProgramID, "uScale");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "uColor");

    // 1. 충돌 시 주인공이 위치한 칸 내부만 배경 효과 적용
    if (collisionEffectTimer > 0.0f) {
        drawCellBackground(player.gridX, player.gridY, collisionEffectColor);
    }

    // 2. 보드판 격자선 그리기
    glUniform2f(transLoc, 0.0f, 0.0f);
    glUniform1f(scaleLoc, 1.0f);
    glUniform4f(colorLoc, 0.2f, 0.2f, 0.2f, 1.0f);
    glBindVertexArray(gridVAO);
    int linePointCount = ((boardWidth + 1) + (boardHeight + 1)) * 2;
    glDrawArrays(GL_LINES, 0, linePointCount);

    // 3. 장애물 출력
    for (int i = 0; i < obstacleCount; ++i) {
        drawEntity(&obstacles[i]);
    }

    // 4. 주인공 출력
    drawEntity(&player);

    glBindVertexArray(0);
}


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    switch (key) {
    case GLFW_KEY_S:
        if (action == GLFW_PRESS) {
            isMoving = !isMoving;
        }
        break;
    case GLFW_KEY_EQUAL: // '+' 키 (Shift 키 입력 포함 또는 키패드 +)
    case GLFW_KEY_KP_ADD:
        moveInterval -= 0.02f; // 이동 간격을 줄여 속도 증가
        if (moveInterval < 0.02f) moveInterval = 0.02f; // 최대 속도 제한
        printf("[속도 증가] 이동 간격: %.2f초/칸\n", moveInterval);
        break;
    case GLFW_KEY_MINUS: // '-' 키 또는 키패드 -
    case GLFW_KEY_KP_SUBTRACT:
        moveInterval += 0.02f; // 이동 간격을 늘려 속도 감소
        if (moveInterval > 0.50f) moveInterval = 0.50f; // 최소 속도 제한
        printf("[속도 감소] 이동 간격: %.2f초/칸\n", moveInterval);
        break;
    case GLFW_KEY_R:
        if (action == GLFW_PRESS) {
            initBoardAndEntities();
        }
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

int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));

    printf("=== 실습 11 보드판 설정 ===\n");
    printf("보드판의 가로(N) 및 세로(M) 크기를 입력하세요 (예: 20 20): ");
    if (scanf("%d %d", &boardWidth, &boardHeight) != 2 || boardWidth < 2 || boardHeight < 2) {
        printf("잘못된 입력입니다. 기본값 20x20으로 설정합니다.\n");
        boardWidth = 20;
        boardHeight = 20;
    }

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "GLSL Practice 11 - Board Game", NULL, NULL);
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
    initBoardAndEntities();

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

        updatePlayer(dt);
        drawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}