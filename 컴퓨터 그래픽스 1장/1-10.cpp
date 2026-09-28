#include <GL/glew.h>
#include <GL/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// ==========================================
// 1. 상수
// ==========================================
#define WIN_W       1000          // 월드 좌표 가로 (픽셀 단위로 사용)
#define WIN_H       700           // 월드 좌표 세로
#define DIVIDER_X   600.0f        // 좌측(도형) / 우측(모양판) 경계
#define UNIT        40.0f         // 도형 기본 크기
#define TRI_H       34.641016f    // 정삼각형 높이 = UNIT * sqrt(3) / 2
#define SNAP_DIST   25.0f         // 이 거리 안에서 놓으면 모양판 칸에 붙음
#define HALF_PI     1.5707963f

#define MAX_PIECES  32
#define MAX_SLOTS   8
#define NUM_BOARDS  5

enum { SHAPE_SQUARE = 0, SHAPE_EQUI = 1, SHAPE_RIGHT = 2, SHAPE_COUNT = 3 };

// ==========================================
// 2. 셰이더
//    - 월드 좌표(픽셀, 원점 좌하단)를 받아서 셰이더 안에서
//      크기 -> 회전 -> 이동 -> NDC 변환까지 처리
// ==========================================
const char* vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec2 vPos;\n"
"uniform vec2  uOffset;\n"     // 도형 중심 위치 (월드)
"uniform vec2  uScale;\n"      // 크기 배율
"uniform float uAngle;\n"      // 회전 각도 (라디안, 반시계)
"uniform vec2  uScreen;\n"     // 월드 크기 (1000, 700)
"void main()\n"
"{\n"
"    float c = cos(uAngle);\n"
"    float s = sin(uAngle);\n"
"    mat2 rot = mat2(c, s, -s, c);\n"          // 열 우선: [c -s; s c]
"    vec2 world = rot * (vPos * uScale) + uOffset;\n"
"    vec2 ndc = world / uScreen * 2.0 - 1.0;\n"
"    gl_Position = vec4(ndc, 0.0, 1.0);\n"
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
// 3. 자료구조
// ==========================================
typedef struct {
    float r, g, b;
} Color;

typedef struct {
    int   type;       // SHAPE_SQUARE / SHAPE_EQUI / SHAPE_RIGHT
    int   rot;        // 회전 단계 0~3 (x 90도, 반시계)
    float x, y;       // 중심 위치 (월드)
    Color color;
    int   board;      // 올라가 있는 모양판 번호 (-1: 없음)
    int   slotIdx;    // 모양판 안의 칸 번호
    int   locked;     // 1이면 완성된 모양판에 고정 -> 이동 불가
} Piece;

typedef struct {
    int   type;
    int   rot;
    float x, y;       // 칸 중심 (월드)
    int   filled;     // 올라간 도형 인덱스 (-1: 비어 있음)
} Slot;

typedef struct {
    float cx, cy;     // 모양판 중심
    float w, h;       // 모양판 전체 크기 (완성 테두리용)
    int   numSlots;
    Slot  slots[MAX_SLOTS];
    int   completed;
} Board;

// 도형별 로컬 정점 (중심 = 바운딩박스 중심, 회전 0 기준)
// 정사각형: 한 변 UNIT
// 정삼각형: 한 변 UNIT, 위쪽을 향함
// 직각삼각형: 밑변 UNIT, 높이 2*UNIT, 직각이 좌하단
float shapeVerts[SHAPE_COUNT][8] = {
    { -20.0f, -20.0f,   20.0f, -20.0f,   20.0f, 20.0f,   -20.0f, 20.0f },
    { -20.0f, -TRI_H / 2,   20.0f, -TRI_H / 2,   0.0f, TRI_H / 2,   0.0f, 0.0f },
    { -20.0f, -40.0f,   20.0f, -40.0f,  -20.0f, 40.0f,   0.0f, 0.0f }
};
int shapeVertCount[SHAPE_COUNT] = { 4, 3, 3 };
unsigned int squareIndices[6] = { 0, 1, 2,   0, 2, 3 };   // EBO: 사각형 = 삼각형 2개

// 90도 단위 회전용 cos/sin (CPU 쪽 충돌 판정에서 사용)
const float COS_T[4] = { 1.0f, 0.0f, -1.0f, 0.0f };
const float SIN_T[4] = { 0.0f, 1.0f,  0.0f, -1.0f };

Piece pieces[MAX_PIECES];
int   pieceCount = 0;
int   drawOrder[MAX_PIECES];   // 그리는 순서 (뒤쪽일수록 위에 그려짐)
Board boards[NUM_BOARDS];

int   dragIdx = -1;            // 드래그 중인 도형 인덱스
float grabDX, grabDY;          // 잡은 지점과 도형 중심의 차이

GLuint shaderProgramID;
GLuint shapeVAO[SHAPE_COUNT], shapeVBO[SHAPE_COUNT], squareEBO;
GLuint lineVAO, lineVBO;
GLint  offsetLoc, scaleLoc, angleLoc, screenLoc, colorLoc;

// ==========================================
// 4. 셰이더 / 버퍼 초기화
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

void initBuffers() {
    // 도형 3종류: 각각 VAO 1개 + VBO 1개 (사각형은 EBO 추가)
    glGenVertexArrays(SHAPE_COUNT, shapeVAO);
    glGenBuffers(SHAPE_COUNT, shapeVBO);

    for (int i = 0; i < SHAPE_COUNT; ++i) {
        glBindVertexArray(shapeVAO[i]);
        glBindBuffer(GL_ARRAY_BUFFER, shapeVBO[i]);
        glBufferData(GL_ARRAY_BUFFER, shapeVertCount[i] * 2 * sizeof(float), shapeVerts[i], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        if (i == SHAPE_SQUARE) {
            // VAO가 바인드된 상태에서 EBO를 바인드해야 VAO에 기록된다
            glGenBuffers(1, &squareEBO);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, squareEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(squareIndices), squareIndices, GL_STATIC_DRAW);
        }
    }

    // 좌/우 구분선
    float lineVerts[] = { DIVIDER_X, 0.0f,   DIVIDER_X, (float)WIN_H };
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVerts), lineVerts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// ==========================================
// 5. 게임 데이터 초기화 (모양판 / 도형)
// ==========================================
float randRange(float a, float b) {
    return a + ((float)rand() / RAND_MAX) * (b - a);
}

Color getRandomColor() {
    Color c;
    c.r = randRange(0.15f, 0.95f);
    c.g = randRange(0.15f, 0.95f);
    c.b = randRange(0.15f, 0.95f);
    return c;
}

void addSlot(Board* b, int type, int rot, float dx, float dy) {
    Slot* s = &b->slots[b->numSlots++];
    s->type = type;
    s->rot = rot;
    s->x = b->cx + dx;
    s->y = b->cy + dy;
    s->filled = -1;
}

// 회전 단계: 0=위, 1=왼쪽, 2=아래, 3=오른쪽 (정삼각형 꼭짓점 방향 기준)
void initBoards() {
    for (int i = 0; i < NUM_BOARDS; ++i) {
        boards[i].cx = DIVIDER_X + (WIN_W - DIVIDER_X) / 2.0f;
        boards[i].cy = WIN_H - 70.0f - 140.0f * i;
        boards[i].numSlots = 0;
        boards[i].completed = 0;
    }

    // [0] 작은 사각형 4개 -> 큰 사각형
    boards[0].w = 80.0f; boards[0].h = 80.0f;
    addSlot(&boards[0], SHAPE_SQUARE, 0, -20.0f, 20.0f);
    addSlot(&boards[0], SHAPE_SQUARE, 0, 20.0f, 20.0f);
    addSlot(&boards[0], SHAPE_SQUARE, 0, -20.0f, -20.0f);
    addSlot(&boards[0], SHAPE_SQUARE, 0, 20.0f, -20.0f);

    // [1] 정삼각형 4개 -> 꼭짓점이 가운데로 모이는 모양
    boards[1].w = 2 * TRI_H; boards[1].h = 2 * TRI_H;
    addSlot(&boards[1], SHAPE_EQUI, 2, 0.0f, TRI_H / 2);   // 위쪽 (아래를 향함)
    addSlot(&boards[1], SHAPE_EQUI, 0, 0.0f, -TRI_H / 2);   // 아래쪽 (위를 향함)
    addSlot(&boards[1], SHAPE_EQUI, 3, -TRI_H / 2, 0.0f);   // 왼쪽 (오른쪽을 향함)
    addSlot(&boards[1], SHAPE_EQUI, 1, TRI_H / 2, 0.0f);   // 오른쪽 (왼쪽을 향함)

    // [2] 직각삼각형 2개 -> 세로 직사각형 (대각선 분할)
    boards[2].w = 40.0f; boards[2].h = 80.0f;
    addSlot(&boards[2], SHAPE_RIGHT, 0, 0.0f, 0.0f);        // 직각이 좌하단
    addSlot(&boards[2], SHAPE_RIGHT, 2, 0.0f, 0.0f);        // 직각이 우상단

    // [3] (추가 1) 집: 사각형 + 정삼각형 지붕
    boards[3].w = 40.0f; boards[3].h = 40.0f + TRI_H;
    addSlot(&boards[3], SHAPE_SQUARE, 0, 0.0f, -TRI_H / 2);
    addSlot(&boards[3], SHAPE_EQUI, 0, 0.0f, 20.0f);

    // [4] (추가 2) 배: 직각삼각형 2개(가로 선체) + 정삼각형 돛
    boards[4].w = 80.0f; boards[4].h = 40.0f + TRI_H;
    addSlot(&boards[4], SHAPE_RIGHT, 1, 0.0f, -TRI_H / 2);  // 직각이 우하단
    addSlot(&boards[4], SHAPE_RIGHT, 3, 0.0f, -TRI_H / 2);  // 직각이 좌상단
    addSlot(&boards[4], SHAPE_EQUI, 0, 0.0f, 20.0f);
}

void addPiece(int type, int rot) {
    if (pieceCount >= MAX_PIECES) return;

    Piece* p = &pieces[pieceCount];
    p->type = type;
    p->rot = rot;
    p->color = getRandomColor();
    p->board = -1;
    p->slotIdx = -1;
    p->locked = 0;

    // 좌측 영역 안의 랜덤 위치 (다른 도형과 너무 겹치지 않게 재시도)
    for (int tries = 0; tries < 300; ++tries) {
        p->x = randRange(60.0f, DIVIDER_X - 60.0f);
        p->y = randRange(60.0f, WIN_H - 60.0f);

        int ok = 1;
        for (int i = 0; i < pieceCount; ++i) {
            float dx = pieces[i].x - p->x;
            float dy = pieces[i].y - p->y;
            if (dx * dx + dy * dy < 75.0f * 75.0f) { ok = 0; break; }
        }
        if (ok) break;
    }
    pieceCount++;
}

void initPieces() {
    pieceCount = 0;

    // 모든 모양판을 채울 수 있도록 필요한 도형은 반드시 생성
    for (int b = 0; b < NUM_BOARDS; ++b)
        for (int s = 0; s < boards[b].numSlots; ++s)
            addPiece(boards[b].slots[s].type, boards[b].slots[s].rot);

    // 여분 도형 0~5개 추가 (랜덤 개수)
    int extra = rand() % 6;
    for (int i = 0; i < extra; ++i)
        addPiece(rand() % SHAPE_COUNT, rand() % 4);

    for (int i = 0; i < pieceCount; ++i)
        drawOrder[i] = i;
}

void resetGame() {
    initBoards();
    initPieces();
    dragIdx = -1;
    printf("리셋: 도형 %d개 생성\n", pieceCount);
}

// ==========================================
// 6. 선택 / 스냅 처리
// ==========================================
float edgeSign(float px, float py, float ax, float ay, float bx, float by) {
    return (px - bx) * (ay - by) - (ax - bx) * (py - by);
}

int pointInTri(float px, float py, float ax, float ay, float bx, float by, float cx, float cy) {
    float d1 = edgeSign(px, py, ax, ay, bx, by);
    float d2 = edgeSign(px, py, bx, by, cx, cy);
    float d3 = edgeSign(px, py, cx, cy, ax, ay);
    int hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    int hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);
    return !(hasNeg && hasPos);
}

// 셰이더와 같은 방식(회전 -> 이동)으로 CPU에서 정점을 계산해서 판정
int hitTest(const Piece* p, float mx, float my) {
    float w[4][2];
    int n = shapeVertCount[p->type];
    float c = COS_T[p->rot], s = SIN_T[p->rot];

    for (int i = 0; i < n; ++i) {
        float lx = shapeVerts[p->type][i * 2];
        float ly = shapeVerts[p->type][i * 2 + 1];
        w[i][0] = p->x + lx * c - ly * s;
        w[i][1] = p->y + lx * s + ly * c;
    }

    if (pointInTri(mx, my, w[0][0], w[0][1], w[1][0], w[1][1], w[2][0], w[2][1])) return 1;
    if (n == 4 && pointInTri(mx, my, w[0][0], w[0][1], w[2][0], w[2][1], w[3][0], w[3][1])) return 1;
    return 0;
}

// 위에 그려진 도형부터 검사 -> drawOrder에서의 위치 반환
int pickPiece(float mx, float my) {
    for (int k = pieceCount - 1; k >= 0; --k) {
        if (hitTest(&pieces[drawOrder[k]], mx, my)) return k;
    }
    return -1;
}

void bringToFront(int k) {
    int idx = drawOrder[k];
    for (int i = k; i < pieceCount - 1; ++i)
        drawOrder[i] = drawOrder[i + 1];
    drawOrder[pieceCount - 1] = idx;
}

void releaseSlot(Piece* p) {
    if (p->board >= 0) {
        boards[p->board].slots[p->slotIdx].filled = -1;
        p->board = -1;
        p->slotIdx = -1;
    }
}

// 사각형은 90도 회전해도 모양이 같으므로 방향 무시
int rotMatch(int type, int a, int b) {
    if (type == SHAPE_SQUARE) return 1;
    return a == b;
}

void checkBoard(int b) {
    Board* bd = &boards[b];
    for (int s = 0; s < bd->numSlots; ++s)
        if (bd->slots[s].filled < 0) return;

    bd->completed = 1;
    for (int s = 0; s < bd->numSlots; ++s)
        pieces[bd->slots[s].filled].locked = 1;   // 더 이상 움직일 수 없음
    printf("모양판 %d 완성!\n", b + 1);

    for (int i = 0; i < NUM_BOARDS; ++i)
        if (!boards[i].completed) return;
    printf("모든 모양판 완성! (r: 새로 시작)\n");
}

void trySnap(int idx) {
    Piece* p = &pieces[idx];
    int bestB = -1, bestS = -1;
    float bestD = SNAP_DIST;

    for (int b = 0; b < NUM_BOARDS; ++b) {
        if (boards[b].completed) continue;
        for (int s = 0; s < boards[b].numSlots; ++s) {
            Slot* sl = &boards[b].slots[s];
            if (sl->filled >= 0) continue;
            if (sl->type != p->type) continue;
            if (!rotMatch(p->type, sl->rot, p->rot)) continue;

            float d = sqrtf((sl->x - p->x) * (sl->x - p->x) + (sl->y - p->y) * (sl->y - p->y));
            if (d < bestD) { bestD = d; bestB = b; bestS = s; }
        }
    }

    if (bestB >= 0) {
        Slot* sl = &boards[bestB].slots[bestS];
        p->x = sl->x;
        p->y = sl->y;
        sl->filled = idx;
        p->board = bestB;
        p->slotIdx = bestS;
        checkBoard(bestB);
    }
}

// ==========================================
// 7. 콜백 함수
// ==========================================
// 창 크기가 바뀌어도 월드 좌표(1000x700) 기준으로 변환
void cursorToWorld(GLFWwindow* window, float* wx, float* wy) {
    double xpos, ypos;
    int w, h;
    glfwGetCursorPos(window, &xpos, &ypos);
    glfwGetWindowSize(window, &w, &h);
    if (w <= 0 || h <= 0) { *wx = *wy = -1000.0f; return; }
    *wx = (float)(xpos / w * WIN_W);
    *wy = (float)(WIN_H - ypos / h * WIN_H);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
    case GLFW_KEY_R:
        resetGame();
        break;
    case GLFW_KEY_Q:
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    float mx, my;
    cursorToWorld(window, &mx, &my);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            int k = pickPiece(mx, my);
            if (k < 0) return;
            int idx = drawOrder[k];
            if (pieces[idx].locked) return;         // 완성된 도형은 선택 불가

            bringToFront(k);
            dragIdx = idx;
            grabDX = pieces[idx].x - mx;
            grabDY = pieces[idx].y - my;
            releaseSlot(&pieces[idx]);               // 칸에서 다시 빼는 경우
        }
        else if (action == GLFW_RELEASE && dragIdx >= 0) {
            trySnap(dragIdx);
            dragIdx = -1;
        }
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS && dragIdx < 0) {
        // 우클릭: 90도 회전 (편의 기능)
        int k = pickPiece(mx, my);
        if (k < 0) return;
        int idx = drawOrder[k];
        if (pieces[idx].locked) return;

        bringToFront(k);
        releaseSlot(&pieces[idx]);
        pieces[idx].rot = (pieces[idx].rot + 1) % 4;
        trySnap(idx);
    }
}

void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos) {
    if (dragIdx < 0) return;

    float mx, my;
    cursorToWorld(window, &mx, &my);

    float nx = mx + grabDX;
    float ny = my + grabDY;
    if (nx < 0.0f) nx = 0.0f;
    if (nx > WIN_W) nx = (float)WIN_W;
    if (ny < 0.0f) ny = 0.0f;
    if (ny > WIN_H) ny = (float)WIN_H;
    pieces[dragIdx].x = nx;
    pieces[dragIdx].y = ny;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

// ==========================================
// 8. 그리기
// ==========================================
void setTransform(float x, float y, float sx, float sy, float angle) {
    glUniform2f(offsetLoc, x, y);
    glUniform2f(scaleLoc, sx, sy);
    glUniform1f(angleLoc, angle);
}

void setColor(float r, float g, float b) {
    glUniform4f(colorLoc, r, g, b, 1.0f);
}

void drawShape(int type, int filled) {
    glBindVertexArray(shapeVAO[type]);
    if (filled) {
        if (type == SHAPE_SQUARE)
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);   // EBO 사용
        else
            glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    else {
        glDrawArrays(GL_LINE_LOOP, 0, shapeVertCount[type]);
    }
}

void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgramID);
    glUniform2f(screenLoc, (float)WIN_W, (float)WIN_H);
    glLineWidth(1.5f);

    // 1. 구분선
    setTransform(0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
    setColor(0.4f, 0.6f, 0.85f);
    glBindVertexArray(lineVAO);
    glDrawArrays(GL_LINES, 0, 2);

    // 2. 모양판 (칸 테두리 + 완성 시 초록 테두리)
    for (int b = 0; b < NUM_BOARDS; ++b) {
        Board* bd = &boards[b];

        if (bd->completed) {
            setTransform(bd->cx, bd->cy, (bd->w + 16.0f) / UNIT, (bd->h + 16.0f) / UNIT, 0.0f);
            setColor(0.1f, 0.75f, 0.25f);
            drawShape(SHAPE_SQUARE, 0);
        }

        for (int s = 0; s < bd->numSlots; ++s) {
            Slot* sl = &bd->slots[s];
            setTransform(sl->x, sl->y, 1.0f, 1.0f, sl->rot * HALF_PI);
            setColor(0.3f, 0.45f, 0.7f);
            drawShape(sl->type, 0);
        }
    }

    // 3. 도형 (drawOrder 순서대로: 마지막이 맨 위)
    for (int k = 0; k < pieceCount; ++k) {
        Piece* p = &pieces[drawOrder[k]];
        setTransform(p->x, p->y, 1.0f, 1.0f, p->rot * HALF_PI);

        setColor(p->color.r, p->color.g, p->color.b);
        drawShape(p->type, 1);

        setColor(0.2f, 0.2f, 0.2f);
        drawShape(p->type, 0);
    }

    glBindVertexArray(0);
}

// ==========================================
// 9. 메인
// ==========================================
int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, "GLSL Practice 10 - Shape Puzzle", NULL, NULL);
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
    offsetLoc = glGetUniformLocation(shaderProgramID, "uOffset");
    scaleLoc = glGetUniformLocation(shaderProgramID, "uScale");
    angleLoc = glGetUniformLocation(shaderProgramID, "uAngle");
    screenLoc = glGetUniformLocation(shaderProgramID, "uScreen");
    colorLoc = glGetUniformLocation(shaderProgramID, "uColor");

    initBuffers();
    resetGame();

    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    int fbW, fbH;
    glfwGetFramebufferSize(window, &fbW, &fbH);
    glViewport(0, 0, fbW, fbH);

    printf("좌클릭 드래그: 도형 이동 / 우클릭: 90도 회전\n");
    printf("r: 리셋 / q: 종료\n");

    while (!glfwWindowShouldClose(window)) {
        DrawScene();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}