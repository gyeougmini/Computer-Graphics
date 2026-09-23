#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>


// 색상 구조체
struct Color {
	float r, g, b;
};

// 사각형 구조체
struct Rect {
	float centerX;      // 중심 X 좌표
	float centerY;      // 중심 Y 좌표
	float r, g, b;		// 색상
	float vx, vy;		// 시간당 이동 거리
	int move;
};

Color quadrantColors[5];
Rect rects[5];

bool press1 = false;
bool press2 = false;
bool press3 = false;
bool press4 = false;
bool press5 = false;

int rectnum = 0;

double lastTime = 0.0f;
const float HALF_SIZE = 0.2f;
const float BASE_SPEED = 0.8f; // 초당 0.8 단위 이동

// 랜덤 색상 반환 (0.0f ~ 1.0f)
Color getRandomColor() {
	return {
		(float)rand() / RAND_MAX,
		(float)rand() / RAND_MAX,
		(float)rand() / RAND_MAX
	};
}

void initQuadrantColors() {
	for (int i = 0; i < 5; ++i) {
		quadrantColors[i] = getRandomColor();
	}
}

void initTimer() {
	lastTime = glfwGetTime();
}


void DrawScene() {

	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	for (int i = 0; i < rectnum; ++i) {
		glColor3f(rects[i].r, rects[i].g, rects[i].b);
		glRectf(rects[i].centerX - 0.15f, rects[i].centerY - 0.2f,
			rects[i].centerX + 0.15f, rects[i].centerY + 0.2f);
	}

}

void diagMove() {
	double currentTime = glfwGetTime();
	float deltaTime = (float)(currentTime - lastTime);
	lastTime = currentTime;

	if (press1) {
			for (int i = 0; i < rectnum; ++i) {
				rects[i].centerX += rects[i].vx * deltaTime;
				rects[i].centerY += rects[i].vy * deltaTime;

				// 좌우 벽 충돌 판정 (-1.0 ~ 1.0)
				if (rects[i].centerX + HALF_SIZE >= 1.0f) {
					rects[i].centerX = 1.0f - HALF_SIZE; // 벽 밖으로 나가지 않도록 보정
					rects[i].vx *= -1.0f;
				}
				else if (rects[i].centerX - HALF_SIZE <= -1.0f) {
					rects[i].centerX = -1.0f + HALF_SIZE;
					rects[i].vx *= -1.0f;
				}

				// 상하 벽 충돌 판정 (-1.0 ~ 1.0)
				if (rects[i].centerY + HALF_SIZE >= 1.0f) {
					rects[i].centerY = 1.0f - HALF_SIZE;
					rects[i].vy *= -1.0f;
				}
				else if (rects[i].centerY - HALF_SIZE <= -1.0f) {
					rects[i].centerY = -1.0f + HALF_SIZE;
					rects[i].vy *= -1.0f;
				}
			}
		}
}

void zigzagMove() {

}

void clockMove() {

}

void sizeMove() {

}

void colorChange() {

}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

	// 프로그램 종료 (q, ESC)
	if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) {
		glfwSetWindowShouldClose(window, true);
	}

	if (key == GLFW_KEY_1) {
		press1 = !press1;
		diagMove();
	}

	if (key == GLFW_KEY_2) {
		glfwSetWindowShouldClose(window, true);
	}

	if (key == GLFW_KEY_3) {
		glfwSetWindowShouldClose(window, true);
	}

	if (key == GLFW_KEY_4) {
		glfwSetWindowShouldClose(window, true);
	}

	if (key == GLFW_KEY_5) {
		glfwSetWindowShouldClose(window, true);
	}


}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		// 윈도우 화면 크기 가져오기
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		// 스크린 좌표 -> OpenGL 정규화 좌표계 (-1.0 ~ 1.0) 변환
		float glX = (float)((xpos / width) * 2.0 - 1.0);
		float glY = (float)(1.0 - (ypos / height) * 2.0); // Y축 반전

		if (rectnum < 5) {
			// 좌표 및 색상 데이터만 저장
			initQuadrantColors();
			rects[rectnum].centerX = glX;
			rects[rectnum].centerY = glY;
			rects[rectnum].r = quadrantColors[rectnum].r;
			rects[rectnum].g = quadrantColors[rectnum].g;
			rects[rectnum].b = quadrantColors[rectnum].b;

			rects[rectnum].vx = (rectnum % 2 == 0) ? BASE_SPEED : -BASE_SPEED;
			rects[rectnum].vy = (rectnum % 3 == 0) ? BASE_SPEED : -BASE_SPEED;

			++rectnum;
		}
	}
}

int main() {

	srand((unsigned)time(NULL));

	//--- GLFW 초기화
	if (!glfwInit()) {
		std::cerr << "GLFW 초기화 실패!" << std::endl;
		return -1;
	}
	//--- OpenGL 버전 설정(예: 3.3 Core Profile)
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
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

	// 콜백 함수 등록
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback); 
	initTimer();

	//--- 뷰포트설정
	glViewport(0, 0, 800, 600);
	//--- 메인 루프
	while (!glfwWindowShouldClose(window)) {
		DrawScene();

		// 버퍼교체
		glfwSwapBuffers(window);
		glfwPollEvents();
	}
	//--- 종료 처리
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}