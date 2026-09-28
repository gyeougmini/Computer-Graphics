#include <GL/glew.h>
#include <GL/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_SHAPES 50

// ==========================================
// 1. GLSL 셰이더 소스코드 (C 문자열)
// ==========================================

// 버텍스 셰이더
const char* vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 vPos;\n"
"uniform vec2 uTranslation;\n"
"void main()\n"
"{\n"
"    gl_Position = vec4(vPos.x + uTranslation.x, vPos.y + uTranslation.y, vPos.z, 1.0);\n"
"}\n";

// 프래그먼트 셰이더
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
    TYPE_POINT,
    TYPE_LINE,
    TYPE_TRIANGLE,
    TYPE_RECT
} ShapeType;

typedef struct {
    float r, g, b;
} Color;

typedef struct {
    ShapeType type;
    float posX, posY; // 도형의 중심 위치
    Color color;
} Shape;

Shape shapes[MAX_SHAPES];
int shapeCount = 0;
int selectedIndex = -1;

GLuint shaderProgramID;
GLuint pointVAO, pointVBO;
GLuint lineVAO, lineVBO;
GLuint triVAO, triVBO;
GLuint rectVAO, rectVBO;

// ==========================================
// 3. 셰이더 생성 및 컴파일 함수
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

// ==========================================
// 4. 버퍼 초기화 (VAO / VBO)
// ==========================================

void initBuffers() {
    // 1. 점 (Point)
    float pointVertices[] = { 0.0f, 0.0f, 0.0f };
    glGenVertexArrays(1, &pointVAO);
    glGenBuffers(1, &pointVBO);
    glBindVertexArray(pointVAO);
    glBindBuffer(GL_ARRAY_BUFFER, pointVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pointVertices), pointVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 2. 선 (Line)
    float lineVertices[] = {
        -0.05f, 0.0f, 0.0f,
         0.05f, 0.0f, 0.0f
    };
    glGenVertexArrays(1, &lineVAO);
    glGenBuffers(1, &lineVBO);
    glBindVertexArray(lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lineVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lineVertices), lineVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 3. 삼각형 (Triangle)
    float triVertices[] = {
         0.0f,   0.08f, 0.0f,
        -0.06f, -0.06f, 0.0f,
         0.06f, -0.06f, 0.0f
    };
    glGenVertexArrays(1, &triVAO);
    glGenBuffers(1, &triVBO);
    glBindVertexArray(triVAO);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVertices), triVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 4. 사각형 (Rectangle - 삼각형 2개)
    float rectVertices[] = {
        -0.06f, -0.06f, 0.0f,
         0.06f, -0.06f, 0.0f,
         0.06f,  0.06f, 0.0f,

        -0.06f, -0.06f, 0.0f,
         0.06f,  0.06f, 0.0f,
        -0.06f,  0.06f, 0.0f
    };
    glGenVertexArrays(1, &rectVAO);
    glGenBuffers(1, &rectVBO);
    glBindVertexArray(rectVAO);
    glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(rectVertices), rectVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// ==========================================
// 5. 이벤트 핸들러 및 렌더링
// ==========================================

void addShape(ShapeType type) {
    if (shapeCount >= MAX_SHAPES) {
        printf("최대 도형 개수(%d개) 초과!\n", MAX_SHAPES);
        return;
    }

    float rx = ((float)rand() / RAND_MAX) * 1.6f - 0.8f;
    float ry = ((float)rand() / RAND_MAX) * 1.6f - 0.8f;

    shapes[shapeCount].type = type;
    shapes[shapeCount].posX = rx;
    shapes[shapeCount].posY = ry;
    shapes[shapeCount].color = getRandomColor();

    selectedIndex = shapeCount; // 최근 추가된 도형 선택
    shapeCount++;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    const float step = 0.03f;

    switch (key) {
        // 도형 생성
    case GLFW_KEY_P: addShape(TYPE_POINT); break;
    case GLFW_KEY_E: addShape(TYPE_LINE); break;
    case GLFW_KEY_T: addShape(TYPE_TRIANGLE); break;
    case GLFW_KEY_R: addShape(TYPE_RECT); break;

        // 선택된 도형 단독 이동 (W/A/S/D)
    case GLFW_KEY_W:
        if (selectedIndex != -1) shapes[selectedIndex].posY += step;
        break;
    case GLFW_KEY_A:
        if (selectedIndex != -1) shapes[selectedIndex].posX -= step;
        break;
    case GLFW_KEY_S:
        if (selectedIndex != -1) shapes[selectedIndex].posY -= step;
        break;
    case GLFW_KEY_D:
        if (selectedIndex != -1) shapes[selectedIndex].posX += step;
        break;

        // 선택된 도형 대각선 이동 (I/J/K/L)
    case GLFW_KEY_I: // 좌상
        if (selectedIndex != -1) { shapes[selectedIndex].posX -= step; shapes[selectedIndex].posY += step; }
        break;
    case GLFW_KEY_J: // 우상
        if (selectedIndex != -1) { shapes[selectedIndex].posX += step; shapes[selectedIndex].posY += step; }
        break;
    case GLFW_KEY_K: // 좌하
        if (selectedIndex != -1) { shapes[selectedIndex].posX -= step; shapes[selectedIndex].posY -= step; }
        break;
    case GLFW_KEY_L: // 우하
        if (selectedIndex != -1) { shapes[selectedIndex].posX += step; shapes[selectedIndex].posY -= step; }
        break;

        // 전체 도형 이동 (1:좌, 2:우, 3:상, 4:하)
    case GLFW_KEY_1:
        for (int i = 0; i < shapeCount; ++i) shapes[i].posX -= step;
        break;
    case GLFW_KEY_2:
        for (int i = 0; i < shapeCount; ++i) shapes[i].posX += step;
        break;
    case GLFW_KEY_3:
        for (int i = 0; i < shapeCount; ++i) shapes[i].posY += step;
        break;
    case GLFW_KEY_4:
        for (int i = 0; i < shapeCount; ++i) shapes[i].posY -= step;
        break;

        // 전체 삭제 및 종료
    case GLFW_KEY_C:
        shapeCount = 0;
        selectedIndex = -1;
        break;
    case GLFW_KEY_Q:
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int width, height;
        glfwGetFramebufferSize(window, &width, &height);

        // 스크린 좌표 -> OpenGL 정규화 좌표 (-1.0 ~ 1.0)
        float glX = (float)((xpos / width) * 2.0 - 1.0);
        float glY = (float)(1.0 - (ypos / height) * 2.0);

        selectedIndex = -1;
        const float range = 0.08f;

        // 가장 최근에 생성된 도형부터 클릭 여부 검사
        for (int i = shapeCount - 1; i >= 0; --i) {
            if (glX >= shapes[i].posX - range && glX <= shapes[i].posX + range &&
                glY >= shapes[i].posY - range && glY <= shapes[i].posY + range) {
                selectedIndex = i;
                break;
            }
        }
    }
}

void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    GLint transLoc = glGetUniformLocation(shaderProgramID, "uTranslation");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "uColor");

    for (int i = 0; i < shapeCount; ++i) {
        glUniform2f(transLoc, shapes[i].posX, shapes[i].posY);
        glUniform4f(colorLoc, shapes[i].color.r, shapes[i].color.g, shapes[i].color.b, 1.0f);

        switch (shapes[i].type) {
        case TYPE_POINT:
            glBindVertexArray(pointVAO);
            glPointSize(10.0f);
            glDrawArrays(GL_POINTS, 0, 1);
            break;

        case TYPE_LINE:
            glBindVertexArray(lineVAO);
            glLineWidth(3.0f);
            glDrawArrays(GL_LINES, 0, 2);
            break;

        case TYPE_TRIANGLE:
            glBindVertexArray(triVAO);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            break;

        case TYPE_RECT:
            glBindVertexArray(rectVAO);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            break;
        }

        // 선택된 도형 강조 테두리
        if (i == selectedIndex) {
            glUniform4f(colorLoc, 0.0f, 0.0f, 0.0f, 1.0f); // 검은색으로 강조
            glPointSize(14.0f);
            glLineWidth(5.0f);

            if (shapes[i].type == TYPE_POINT) {
                glBindVertexArray(pointVAO);
                glDrawArrays(GL_POINTS, 0, 1);
            }
            else if (shapes[i].type == TYPE_LINE) {
                glBindVertexArray(lineVAO);
                glDrawArrays(GL_LINES, 0, 2);
            }
            else if (shapes[i].type == TYPE_TRIANGLE) {
                glBindVertexArray(triVAO);
                glDrawArrays(GL_LINE_LOOP, 0, 3);
            }
            else if (shapes[i].type == TYPE_RECT) {
                glBindVertexArray(rectVAO);
                glDrawArrays(GL_LINE_LOOP, 0, 6);
            }
        }
    }
}

// ==========================================
// 6. 메인 함수
// ==========================================

int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "GLSL Practice 7 - Modern OpenGL", NULL, NULL);
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

    while (!glfwWindowShouldClose(window)) {
        DrawScene();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}