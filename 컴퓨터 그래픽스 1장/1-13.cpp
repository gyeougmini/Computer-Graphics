#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint shaderProgramID;
GLuint cubevao, cubevbo[2];
GLuint pyramidvao, pyramidvbo[2];
GLuint axisvao, axisvbo[2];


std::random_device rd;
std::mt19937 gen(rd());

int randomCubeFaces[2] = { -1, -1 };   // C 키 눌렀을 때 그릴 정육면체 2개 면 (0~5)
int randomPyramidFace = -1;            // T 키 눌렀을 때 그릴 피라미드 옆면 1개 (0~3)
bool isRandomCube = false;
bool isRandomPyramid = false;

// 그려야 할 상태 관리 변수 (-1: 전체 그리기, 0~5: 각 면 그리기)
int drawCubeFace = -1;
int drawPyramidFace = -1;

// 셰이더 파일 읽기 함수
std::string filetobuf(const char* file) {
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
}

void InitShader() {
    std::string vStr = filetobuf("vertex13.glsl");
    std::string fStr = filetobuf("fragment13.glsl");
    const char* vSrc = vStr.c_str();
    const char* fSrc = fStr.c_str();

    GLuint vShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vShader, 1, &vSrc, NULL);
    glCompileShader(vShader);

    GLuint fShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fShader, 1, &fSrc, NULL);
    glCompileShader(fShader);

    shaderProgramID = glCreateProgram();
    glAttachShader(shaderProgramID, vShader);
    glAttachShader(shaderProgramID, fShader);
    glLinkProgram(shaderProgramID);

    glDeleteShader(vShader);
    glDeleteShader(fShader);
}

void InitBuffer() {
    GLfloat cubeShape[36][3] = {
        // 앞면
        {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
        { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f},
        // 뒷면
        {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},
        { 0.5f, 0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f},
        // 윗면
        {-0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
        { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
        // 밑면
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
        { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f,-0.5f},
        // 오른면
        { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f},
        { 0.5f, 0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f},
        // 왼면
        {-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
        {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}
    };

    GLfloat pyramid[18][3] = {
        // 아랫면
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
        { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f,-0.5f},
        // 앞면
        { 0.5f, -0.5f, 0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.0f, 0.5f, 0.0f},
        // 뒷면
        {-0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, 0.5f}, { 0.0f, 0.5f, 0.0f},
        // 오른면
        {0.5f, -0.5f, -0.5f}, {-0.5f, -0.5f, -0.5f}, { 0.0f, 0.5f, 0.0f},
        // 왼면
        {-0.5f, -0.5f, 0.5f}, {0.5f, -0.5f, 0.5f}, { 0.0f, 0.5f, 0.0f}
    };

    GLfloat cubeColors[36][3];
    for (int i = 0; i < 36; i++) {
        int face = i / 6;
        if (face == 0) { cubeColors[i][0] = 1.0f; cubeColors[i][1] = 0.0f; cubeColors[i][2] = 0.0f; } // 빨강
        else if (face == 1) { cubeColors[i][0] = 0.0f; cubeColors[i][1] = 1.0f; cubeColors[i][2] = 0.0f; } // 초록
        else if (face == 2) { cubeColors[i][0] = 0.0f; cubeColors[i][1] = 0.0f; cubeColors[i][2] = 1.0f; } // 파랑
        else if (face == 3) { cubeColors[i][0] = 1.0f; cubeColors[i][1] = 1.0f; cubeColors[i][2] = 0.0f; } // 노랑
        else if (face == 4) { cubeColors[i][0] = 1.0f; cubeColors[i][1] = 0.0f; cubeColors[i][2] = 1.0f; } // 자홍
        else { cubeColors[i][0] = 0.0f; cubeColors[i][1] = 1.0f; cubeColors[i][2] = 1.0f; } // 청록
    }

    GLfloat pyramidColors[18][3];
    for (int i = 0; i < 18; i++) {
        if (i < 6) {
            pyramidColors[i][0] = 0.5f; pyramidColors[i][1] = 0.5f; pyramidColors[i][2] = 0.5f;  // 회색
        }
        else {
            int sideFace = (i - 6) / 3;
            if (sideFace == 0) { pyramidColors[i][0] = 1.0f; pyramidColors[i][1] = 0.0f; pyramidColors[i][2] = 0.0f; } // 빨강
            else if (sideFace == 1) { pyramidColors[i][0] = 0.0f; pyramidColors[i][1] = 1.0f; pyramidColors[i][2] = 0.0f; } // 초록
            else if (sideFace == 2) { pyramidColors[i][0] = 0.0f; pyramidColors[i][1] = 0.0f; pyramidColors[i][2] = 1.0f; } // 파랑
            else { pyramidColors[i][0] = 1.0f; pyramidColors[i][1] = 1.0f; pyramidColors[i][2] = 0.0f; } // 노랑
        }
    }

    GLfloat axisShape[6][3] = {
        {-2.0f, 0.0f, 0.0f}, { 2.0f, 0.0f, 0.0f}, // X축 (-2 ~ +2)
        { 0.0f,-2.0f, 0.0f}, { 0.0f, 2.0f, 0.0f}, // Y축 (-2 ~ +2)
        { 0.0f, 0.0f,-2.0f}, { 0.0f, 0.0f, 2.0f}  // Z축 (-2 ~ +2)
    };

    GLfloat axisColors[6][3] = {
        {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, // X축: 빨강
        {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, // Y축: 초록
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}  // Z축: 파랑
    };

    // -------------------- 정육면체 --------------------
    glGenVertexArrays(1, &cubevao);
    glBindVertexArray(cubevao);

    glGenBuffers(2, cubevbo);

    // 0번 VBO: Position
    glBindBuffer(GL_ARRAY_BUFFER, cubevbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeShape), cubeShape, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // 1번 VBO: Color
    glBindBuffer(GL_ARRAY_BUFFER, cubevbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeColors), cubeColors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
    // -------------------- 정육면체 --------------------


    // -------------------- 피라미드 --------------------
    glGenVertexArrays(1, &pyramidvao);
    glBindVertexArray(pyramidvao);

    glGenBuffers(2, pyramidvbo);

    // 0번 VBO: Position
    glBindBuffer(GL_ARRAY_BUFFER, pyramidvbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramid), pyramid, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // 1번 VBO: Color
    glBindBuffer(GL_ARRAY_BUFFER, pyramidvbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidColors), pyramidColors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
    // -------------------- 피라미드 --------------------

    // -------------------- XYZ 좌표축 VBO 설정 --------------------
    glGenVertexArrays(1, &axisvao);
    glBindVertexArray(axisvao);
    glGenBuffers(2, axisvbo);

    glBindBuffer(GL_ARRAY_BUFFER, axisvbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisShape), axisShape, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, axisvbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisColors), axisColors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
    // -------------------- XYZ 좌표축 VBO 설정 --------------------
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    // 키 입력 시 상태 리셋
    isRandomCube = false;
    isRandomPyramid = false;

    // 키 입력 시 이전 상태 초기화
    switch (key) {
    case GLFW_KEY_Q:
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

        // 정육면체 면 선택 (1~6)
    case GLFW_KEY_1: drawCubeFace = 0; drawPyramidFace = -2; break;
    case GLFW_KEY_2: drawCubeFace = 1; drawPyramidFace = -2; break;
    case GLFW_KEY_3: drawCubeFace = 2; drawPyramidFace = -2; break;
    case GLFW_KEY_4: drawCubeFace = 3; drawPyramidFace = -2; break;
    case GLFW_KEY_5: drawCubeFace = 4; drawPyramidFace = -2; break;
    case GLFW_KEY_6: drawCubeFace = 5; drawPyramidFace = -2; break;

        // 피라미드 면 선택 (7~0)
    case GLFW_KEY_7: drawPyramidFace = 0; drawCubeFace = -2; break;
    case GLFW_KEY_8: drawPyramidFace = 1; drawCubeFace = -2; break;
    case GLFW_KEY_9: drawPyramidFace = 2; drawCubeFace = -2; break;
    case GLFW_KEY_0: drawPyramidFace = 3; drawCubeFace = -2; break;

        // 정육면체 랜덤한 2개 면 선택 (C 키)
    case GLFW_KEY_C: {
        std::uniform_int_distribution<int> uid(0, 5); // 0~5 범위 (6개 면)
        randomCubeFaces[0] = uid(gen);
        do {
            randomCubeFaces[1] = uid(gen);
        } while (randomCubeFaces[0] == randomCubeFaces[1]); // 서로 다른 2개 면 뽑기

        isRandomCube = true;
        drawCubeFace = -2;
        drawPyramidFace = -2;

        std::cout << "Cube Random Faces: " << randomCubeFaces[0] + 1
            << ", " << randomCubeFaces[1] + 1 << std::endl;
        break;
    }

                   // 피라미드 바닥 + 랜덤 1개 옆면 선택 (T 키)
    case GLFW_KEY_T: {
        std::uniform_int_distribution<int> uid(0, 3); // 옆면 0~3 범위
        randomPyramidFace = uid(gen);

        isRandomPyramid = true;
        drawCubeFace = -2;
        drawPyramidFace = -2;

        std::cout << "Pyramid Base + Random Side Face: " << randomPyramidFace + 1 << std::endl;
        break;
    }
    }
}

void drawScene(GLFWwindow* window) {
    // 깊이 버퍼와 색상 버퍼 초기화
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    // 1. Model Matrix: 회전(rotate) 없이 기본 단위 행렬 유지
    glm::mat4 model = glm::mat4(1.0f);

    // 2. View Matrix: 정육면체가 입체적으로 보이도록 카메라를 대각선 방향(X: 1.5, Y: 1.5, Z: 1.5)에 배치
    // glm::lookAt(카메라 위치, 바라보는 목표 지점, 카메라의 위쪽 방향)
    glm::mat4 view = glm::lookAt(
        glm::vec3(1.5f, 1.5f, 2.0f), // 카메라를 사선 위에 위치시킴
        glm::vec3(0.0f, 0.0f, 0.0f), // 원점(정육면체 중심)을 바라봄
        glm::vec3(0.0f, 1.0f, 0.0f)  // Y축을 위쪽 방향으로 설정
    );

    // 3. Projection Matrix: 원근 투영 설정
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 600.0f / 600.0f, 0.1f, 100.0f);

    // 최종 Transform 행렬 (P * V * M)
    glm::mat4 MVP = projection * view * model;

    // Uniform 변수로 Vertex Shader에 전달
    GLuint transformLoc = glGetUniformLocation(shaderProgramID, "transform");
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(MVP));

    glBindVertexArray(cubevao);
    glBindVertexArray(pyramidvao);


    // XYZ 좌표축 렌더링 (GL_LINES 이용)
    glBindVertexArray(axisvao);
    glLineWidth(1.0f); // 선 두께 설정
    glDrawArrays(GL_LINES, 0, 6);

    // 정육면체 렌더링
    glBindVertexArray(cubevao);
    if (isRandomCube) {
        glDrawArrays(GL_TRIANGLES, randomCubeFaces[0] * 6, 6);
        glDrawArrays(GL_TRIANGLES, randomCubeFaces[1] * 6, 6);
    }
    else if (drawCubeFace != -2) {
        if (drawCubeFace == -1) glDrawArrays(GL_TRIANGLES, 0, 36);
        else glDrawArrays(GL_TRIANGLES, drawCubeFace * 6, 6);
    }

    // 피라미드 렌더링
    glBindVertexArray(pyramidvao);
    if (isRandomPyramid) {
        glDrawArrays(GL_TRIANGLES, 0, 6); // 바닥면 (사각형)
        glDrawArrays(GL_TRIANGLES, 6 + randomPyramidFace * 3, 3); // 랜덤 옆면 1개
    }
    else if (drawPyramidFace != -2) {
        if (drawPyramidFace == -1) glDrawArrays(GL_TRIANGLES, 0, 18);
        else glDrawArrays(GL_TRIANGLES, 6 + drawPyramidFace * 3, 3);
    }
}
int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(600, 600, "OpenGL Triangle Example", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;
    glEnable(GL_DEPTH_TEST);

    InitShader();
    InitBuffer();

    glfwSetKeyCallback(window, key_callback);

    while (!glfwWindowShouldClose(window)) {
        drawScene(window);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
