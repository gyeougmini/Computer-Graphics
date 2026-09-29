#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

//--- 필요한 변수 선언
GLint width = 500, height = 500;
GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;
GLuint vao, vbo[2], ebo;

// 배경색 변수
float rColor = 1.0f, gColor = 1.0f, bColor = 1.0f;

// 삼각형 좌표 데이터
float triShape[] = {
    -0.5f, 0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
    -0.5f, -0.5f, 0.0f,
    - 0.5f,  0.5f, 0.0f,
     0.5f,  0.5f, 0.0f,
     0.5f, -0.5f, 0.0f
};

// 삼각형 색상 데이터
float colors[] = {
    1.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f
};

//--- 사용자 정의 함수 선언
std::string filetobuf(const char* file);
void make_vertexShaders();
void make_fragmentShaders();
int make_shaderProgram();
void InitBuffer();
GLvoid drawScene();
GLvoid Reshape(GLFWwindow* window, int w, int h);

// 텍스트 파일 읽기 함수
std::string filetobuf(const char* file)
{
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open()) {
        std::cerr << "파일을 열 수 없습니다: " << file << std::endl;
        return "";
    }
    std::stringstream shaderStream;
    shaderStream << shaderFile.rdbuf();
    shaderFile.close();
    return shaderStream.str();
}

int main(int argc, char** argv)
{
    //--- GLFW 초기화
    if (!glfwInit())
        return -1;

    //--- OpenGL 버전 및 프로파일 설정
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //--- 윈도우 생성
    GLFWwindow* window = glfwCreateWindow(width, height, "Example1", nullptr, nullptr);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }

    //--- 생성한 윈도우의 OpenGL Context를 현재 Context로 설정
    glfwMakeContextCurrent(window);

    // 리사이즈 콜백 설정
    glfwSetFramebufferSizeCallback(window, Reshape);

    //--- GLEW 초기화
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "GLEW 초기화 실패" << std::endl;
        return -1;
    }

    //--- 셰이더 생성
    make_vertexShaders();
    make_fragmentShaders();
    shaderProgramID = make_shaderProgram();

    //--- VBO, VAO 생성
    InitBuffer();

    //--- 렌더링 루프
    while (!glfwWindowShouldClose(window))
    {
        drawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // 종료 처리
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(2, vbo);
    glDeleteProgram(shaderProgramID);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

GLvoid Reshape(GLFWwindow* window, int w, int h)
{
    glViewport(0, 0, w, h);
}

GLvoid drawScene()
{
    glClearColor(rColor, gColor, bColor, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shaderProgramID);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void make_vertexShaders()
{
    std::string vertexSource = filetobuf("vertex.glsl");
    const char* source = vertexSource.c_str();

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &source, NULL);
    glCompileShader(vertexShader);

    GLint result;
    GLchar errorLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, errorLog);
        std::cerr << "ERROR: vertex shader 컴파일 실패\n" << errorLog << std::endl;
    }
}

void make_fragmentShaders()
{
    std::string fragmentSource = filetobuf("fragment.glsl");
    const char* source = fragmentSource.c_str();

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &source, NULL);
    glCompileShader(fragmentShader);

    GLint result;
    GLchar errorLog[512];
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &result);
    if (!result)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, errorLog);
        std::cerr << "ERROR: fragment shader 컴파일 실패\n" << errorLog << std::endl;
    }
}

void InitBuffer()
{
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(2, vbo);

    glGenBuffers(1, &ebo);

    // 1번째 VBO (좌표)
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triShape), triShape, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 2번째 VBO (색상)
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(colors), colors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

int make_shaderProgram()
{
    shaderProgramID = glCreateProgram();
    glAttachShader(shaderProgramID, vertexShader);
    glAttachShader(shaderProgramID, fragmentShader);
    glLinkProgram(shaderProgramID);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgramID;
}