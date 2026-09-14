#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>

int main() {

	srand((unsigned)time(NULL));

	double red = 1.0f;
	double green = 1.0f;
	double blue = 1.0f;

	bool isTimerRunning = false;
	double lastTime = 0.0;
	const double interval = 1.5;

	//--- GLFW 초기화
	if (!glfwInit()) {
		std::cerr << "GLFW 초기화 실패!" << std::endl;
		return -1;
	}
	//--- OpenGL 버전 설정(예: 3.3 Core Profile)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	//--- 윈도우생성
	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Window", nullptr, nullptr);
	if (!window) {
		std::cerr << "윈도우생성실패!" << std::endl;
		glfwTerminate();
		return -1;
	}
	//--- 컨텍스트설정
	glfwMakeContextCurrent(window);
	//--- GLEW 초기화
	glewExperimental = GL_TRUE; // 최신 기능 사용
	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW 초기화 실패!" << std::endl;
		return -1;
	}

	//--- 뷰포트설정
	glViewport(0, 0, 800, 600);
	//--- 메인 루프
	while (!glfwWindowShouldClose(window)) {
		// 입력처리
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
			red = 0.0f; green = 1.0f; blue = 1.0f; // 청록색
		}
		if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
			red = 1.0f; green = 0.0f; blue = 1.0f; // 자홍색
		}
		if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
			red = 1.0f; green = 1.0f; blue = 0.0f; // 노란색
		}
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
			red = (float)rand() / RAND_MAX;
			green = (float)rand() / RAND_MAX;
			blue = (float)rand() / RAND_MAX;
		}
		if (glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS) {
			red = 0.5f; green = 0.5f; blue = 0.5f; // 회색
		}
		if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
			red = 0.0f; green = 0.0f; blue = 0.0f; // 검정색
		}
		if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
			if (!isTimerRunning) {
				isTimerRunning = true;
				lastTime = glfwGetTime(); // 현재 시간 기준점으로 초기화
			}
		}

		// [단축키 s: 타이머 종료]
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
			isTimerRunning = false;
		}

		// [타이머 동작 처리]
		if (isTimerRunning) {
			double currentTime = glfwGetTime();
			if (currentTime - lastTime >= interval) {
				red = (float)rand() / RAND_MAX;
				green = (float)rand() / RAND_MAX;
				blue = (float)rand() / RAND_MAX;
				lastTime = currentTime; // 기준 시간 갱신
			}
		}
		glClearColor(red, green, blue, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// 버퍼교체
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	//--- 종료 처리
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}