//--- 필요한헤더파일선언
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <fstream>
#include <string>

//--- 사용자정의함수
void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);
//--- 필요한변수선언
GLint width, height;
GLuint shaderProgramID;
GLuint vertexShader;
GLuint fragmentShader;
//--- 메인 함수
int main(int argc, char** argv)
{
	width = 500;
	height = 500;
	//--- GLFW 초기화
	if (!glfwInit())
		return -1;
	//--- OpenGL 버전 및 프로파일설정
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//--- 윈도우생성
	GLFWwindow* window =
		glfwCreateWindow(width, height, "Example1", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		return -1;
		//--- 생성한윈도우의OpenGL Context를현재Context로설정
		glfwMakeContextCurrent(window);
		//--- GLEW 초기화
		glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK)
		{
		}
		glfwDestroyWindow(window);
		glfwTerminate();
		return -1;
		//--- 세이더읽어와서세이더프로그램만들기
		make_vertexShaders();
		make_fragmentShaders();
		shaderProgramID = make_shaderProgram();
		//--- 렌더링루프
		while (!glfwWindowShouldClose(window))
		{
		}
		drawScene();
		//--- 윈도우에렌더링결과출력
		glfwSwapBuffers(window);
		//--- 이벤트처리
		glfwPollEvents();
		//--- 종료
		glfwDestroyWindow(window);
		glfwTerminate();
		return 0;
	}
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
		return;
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
		return;
	}
}


GLuint make_shaderProgram()
{
	GLint result;
	GLchar errorLog[512];
	//--- 셰이더프로그램생성, 연결, 링크, 셰이더객체삭제
	GLuint shaderID = glCreateProgram();
	glAttachShader(shaderID, vertexShader);
	glAttachShader(shaderID, fragmentShader);
	glLinkProgram(shaderID);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
	//--- 링크 성공여부확인
	glGetProgramiv(shaderID, GL_LINK_STATUS, &result);
	if (!result) {
		glGetProgramInfoLog(shaderID, 512, NULL, errorLog);
		std::cerr << "ERROR: shader program 연결 실패\n" << errorLog << std::endl;
		return 0;
	}
	//--- 셰이더프로그램사용
	glUseProgram(shaderID);
	return shaderID;
}


void drawScene()
{
	GLfloat rColor, gColor, bColor;
	rColor = gColor = 0.0f;
	bColor = 1.0f;
	//--- 배경색을파란색으로설정
	glClearColor(rColor, gColor, bColor, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	//--- 셰이더프로그램사용
	glUseProgram(shaderProgramID);
	//--- 점의 크기설정
	glPointSize(5.0f);
		//--- 0번 인덱스에서3개의버텍스를사용하여삼각형그리기
		glDrawArrays(GL_POINTS, 0, 3);
	// glDrawArrays(GL_TRIANGLES, 0, 3);
	// glDrawArrays(GL_LINE_LOOP, 0, 3);
}


std::string filetobuf(const char* file)
{
	std::ifstream shaderFile(file);
	if (!shaderFile.is_open())
	{
		return "";
	}
	//--- 파일전체를문자열로읽기
	std::string source((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
	shaderFile.close();
	return source;
}
