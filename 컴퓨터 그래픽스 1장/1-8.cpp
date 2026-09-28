#include <GL/glew.h>
#include <GL/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 버텍스 셰이더
const char* vertexShaderSource =
"#version 330 core\n"
"layout (location = 0) in vec3 vPos;\n"
"uniform vec2 uTranslation;\n"
"uniform float uScale;\n"
"void main()\n"
"{\n"
"    vec3 pos = vPos * uScale;\n"
"    gl_Position = vec4(pos.x + uTranslation.x, pos.y + uTranslation.y, pos.z, 1.0);\n"
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


typedef struct {
    float r, g, b;
} Color;

typedef struct {
    int active;        // 해당 사분면에 삼각형 존재 여부 (1: 있음, 0: 없음)
    float posX, posY;  // 중심 위치 (NDC 좌표)
    float scale;       // 크기 배율
    Color color;       // 색상
    int isScalingUp;   // 우클릭 시 확대/축소 상태 토글용
} Triangle;

// 4개 사분면 관리를 위한 배열 (0: 1사분면, 1: 2사분면, 2: 3사분면, 3: 4사분면)
Triangle triQuadrants[4];

// 폴리곤 모드 (GL_FILL: 면, GL_LINE: 선)
GLenum polygonMode = GL_FILL;

GLuint shaderProgramID;
GLuint triVAO, triVBO;
GLuint axisVAO, axisVBO;


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
    // 1. 기본 이등변삼각형 정점
    float triVertices[] = {
         0.0f,   0.12f, 0.0f,  // 상단
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

    // 2. 사분면 구분용 십자선 정점
    float axisVertices[] = {
        -1.0f,  0.0f, 0.0f,   1.0f,  0.0f, 0.0f,  // X축
         0.0f, -1.0f, 0.0f,   0.0f,  1.0f, 0.0f   // Y축
    };

    glGenVertexArrays(1, &axisVAO);
    glGenBuffers(1, &axisVBO);
    glBindVertexArray(axisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisVertices), axisVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

// MSVC 컴파일 오류(C4576) 수정을 위해 각 멤버별 개별 대입 구조로 변경
void initTriangles() {
    // 1사분면 (우상단)
    triQuadrants[0].active = 1;
    triQuadrants[0].posX = 0.5f;
    triQuadrants[0].posY = 0.5f;
    triQuadrants[0].scale = 1.0f;
    triQuadrants[0].color = getRandomColor();
    triQuadrants[0].isScalingUp = 1;

    // 2사분면 (좌상단)
    triQuadrants[1].active = 1;
    triQuadrants[1].posX = -0.5f;
    triQuadrants[1].posY = 0.5f;
    triQuadrants[1].scale = 1.2f;
    triQuadrants[1].color = getRandomColor();
    triQuadrants[1].isScalingUp = 1;

    // 3사분면 (좌하단)
    triQuadrants[2].active = 1;
    triQuadrants[2].posX = -0.5f;
    triQuadrants[2].posY = -0.5f;
    triQuadrants[2].scale = 0.9f;
    triQuadrants[2].color = getRandomColor();
    triQuadrants[2].isScalingUp = 1;

    // 4사분면 (우하단)
    triQuadrants[3].active = 1;
    triQuadrants[3].posX = 0.5f;
    triQuadrants[3].posY = -0.5f;
    triQuadrants[3].scale = 1.1f;
    triQuadrants[3].color = getRandomColor();
    triQuadrants[3].isScalingUp = 1;
}

// ==========================================
// 4. 좌표 판별 및 콜백 함수
// ==========================================

int getQuadrantIndex(float x, float y) {
    if (x >= 0.0f && y >= 0.0f) return 0; // 1사분면
    if (x < 0.0f && y >= 0.0f) return 1; // 2사분면
    if (x < 0.0f && y < 0.0f)  return 2; // 3사분면
    return 3;                             // 4사분면
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
    case GLFW_KEY_A:
        polygonMode = GL_FILL;
        break;
    case GLFW_KEY_B:
        polygonMode = GL_LINE;
        break;
    case GLFW_KEY_C:
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

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    float glX = (float)((xpos / width) * 2.0 - 1.0);
    float glY = (float)(1.0 - (ypos / height) * 2.0);

    int quadIdx = getQuadrantIndex(glX, glY);

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        triQuadrants[quadIdx].active = 1;
        triQuadrants[quadIdx].posX = glX;
        triQuadrants[quadIdx].posY = glY;
        triQuadrants[quadIdx].scale = 0.6f + ((float)rand() / RAND_MAX) * 1.0f;
        triQuadrants[quadIdx].color = getRandomColor();
    }
    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (triQuadrants[quadIdx].active) {
            if (triQuadrants[quadIdx].isScalingUp) {
                triQuadrants[quadIdx].scale *= 1.4f;
                if (triQuadrants[quadIdx].scale > 2.2f) {
                    triQuadrants[quadIdx].scale = 2.2f;
                    triQuadrants[quadIdx].isScalingUp = 0;
                }
            }
            else {
                triQuadrants[quadIdx].scale *= 0.7f;
                if (triQuadrants[quadIdx].scale < 0.4f) {
                    triQuadrants[quadIdx].scale = 0.4f;
                    triQuadrants[quadIdx].isScalingUp = 1;
                }
            }
        }
    }
}


void DrawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    GLint transLoc = glGetUniformLocation(shaderProgramID, "uTranslation");
    GLint scaleLoc = glGetUniformLocation(shaderProgramID, "uScale");
    GLint colorLoc = glGetUniformLocation(shaderProgramID, "uColor");

    // 1. 사분면 구분선 렌더링
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glUniform2f(transLoc, 0.0f, 0.0f);
    glUniform1f(scaleLoc, 1.0f);
    glUniform4f(colorLoc, 0.7f, 0.7f, 0.7f, 1.0f);

    glBindVertexArray(axisVAO);
    glLineWidth(1.5f);
    glDrawArrays(GL_LINES, 0, 4);

    // 2. 이등변삼각형 렌더링
    glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
    glBindVertexArray(triVAO);

    for (int i = 0; i < 4; ++i) {
        if (triQuadrants[i].active) {
            glUniform2f(transLoc, triQuadrants[i].posX, triQuadrants[i].posY);
            glUniform1f(scaleLoc, triQuadrants[i].scale);
            glUniform4f(colorLoc, triQuadrants[i].color.r, triQuadrants[i].color.g, triQuadrants[i].color.b, 1.0f);

            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }

    glBindVertexArray(0);
}


int main(int argc, char** argv) {
    srand((unsigned int)time(NULL));

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "GLSL Practice 8 - Modern OpenGL", NULL, NULL);
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
    initTriangles();

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