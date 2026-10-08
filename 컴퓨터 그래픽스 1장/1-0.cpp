#include <iostream>
#include <fstream>
#include <string>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


GLuint shaderProgramID;
GLuint vao, vbo[2];

// 셰이더 파일 읽기 함수
std::string filetobuf(const char* file) {
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
}

void InitShader() {
    std::string vStr = filetobuf("vertex.glsl");
    std::string fStr = filetobuf("fragment.glsl");
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
        int face = i / 3;
        if (face == 0 || face == 1) { pyramidColors[i][0] = 1.0f; pyramidColors[i][1] = 0.0f; pyramidColors[i][2] = 0.0f; } // 빨강
        else if (face == 2) { pyramidColors[i][0] = 0.0f; pyramidColors[i][1] = 1.0f; pyramidColors[i][2] = 0.0f; } // 초록
        else if (face == 3) { pyramidColors[i][0] = 0.0f; pyramidColors[i][1] = 0.0f; pyramidColors[i][2] = 1.0f; } // 파랑
        else if (face == 4) { pyramidColors[i][0] = 1.0f; pyramidColors[i][1] = 1.0f; pyramidColors[i][2] = 0.0f; } // 노랑
        else if (face == 5) { pyramidColors[i][0] = 1.0f; pyramidColors[i][1] = 0.0f; pyramidColors[i][2] = 1.0f; } // 자홍
        else { pyramidColors[i][0] = 0.0f; pyramidColors[i][1] = 1.0f; pyramidColors[i][2] = 1.0f; } // 청록
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(2, vbo);

    // 0번 VBO: Position
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramid), pyramid, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    // 1번 VBO: Color
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
        break;
    case GLFW_KEY_Q:
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

    case GLFW_KEY_1:
        break;

    case GLFW_KEY_2:
        break;

    case GLFW_KEY_3:
        break;

    case GLFW_KEY_4:
        break;

    case GLFW_KEY_5:
        break;

    case GLFW_KEY_6:
        break;
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

    glBindVertexArray(vao);

    // --- 키보드 입력 체크 및 부분 렌더링 ---
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
        glDrawArrays(GL_TRIANGLES, 0, 6);   // 1번: 앞면
    }
    else if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
        glDrawArrays(GL_TRIANGLES, 6, 6);   // 2번: 뒷면
    }
    else if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
        glDrawArrays(GL_TRIANGLES, 12, 6);  // 3번: 윗면
    }
    else if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS) {
        glDrawArrays(GL_TRIANGLES, 18, 6);  // 4번: 밑면
    }
    else if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS) {
        glDrawArrays(GL_TRIANGLES, 24, 6);  // 5번: 오른면
    }
    else if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS) {
        glDrawArrays(GL_TRIANGLES, 30, 6);  // 6번: 왼면
    }
    else {
        // 아무 키도 누르지 않았을 때 (기본 전체 정육면체 출력)
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }

    if (glfwGetKey(window, GLFW_KEY_7) == GLFW_PRESS) {
        glDrawArrays(GL_TRIANGLES, 6, 3);   // 1번: 앞면
    }
    else if (glfwGetKey(window, GLFW_KEY_8) == GLFW_PRESS) {
        glDrawArrays(GL_TRIANGLES, 9, 3);   // 2번: 뒷면
    }
    else if (glfwGetKey(window, GLFW_KEY_9) == GLFW_PRESS) {
        glDrawArrays(GL_TRIANGLES, 12, 3);  // 3번: 윗면
    }
    else if (glfwGetKey(window, GLFW_KEY_0) == GLFW_PRESS) {
        glDrawArrays(GL_TRIANGLES, 15, 3);  // 4번: 밑면
    }
    else {
        // 아무 키도 누르지 않았을 때 (기본 전체 정육면체 출력)
        glDrawArrays(GL_TRIANGLES, 0, 18);
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
