#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define MAX_RECTS 100 // 초기 사각형 + 추가 생성 사각형 최대 개수

// 색상 구조체
struct Color {
	float r, g, b;
};

// 사각형 구조체
struct Rect {
	float centerX, centerY; // 중심 좌표 (NDC -1.0 ~ 1.0)
	Color color;            // 색상
	bool visible;           // 화면 표시 여부 (지우개에 지워졌는지 판정)
};

// 화면 내 사각형 배열 및 개수
Rect rects[MAX_RECTS];
int initialRectCount = 0;   // 최초 생성된 사각형 수 (20~40개)
int totalRectCount = 0;     // 현재 총 사각형 수
int addedRectCount = 0;     // 우클릭으로 추가된 사각형 수 (최대 10개)

// 지우개 관련 변수
bool isEraserActive = false;
float eraserX = 0.0f, eraserY = 0.0f;
Color eraserColor = { 0.0f, 0.0f, 0.0f }; // 초기 지우개 색상 (검정)

const float RECT_HALF_SIZE = 0.04f;      // 일반 사각형 반 크기 (Half Width/Height)
float initialEraserHalfSize = RECT_HALF_SIZE * 2.0f; // 지우개 기본 크기 (일반 사각형의 2배)
float currentEraserHalfSize = RECT_HALF_SIZE * 2.0f; // 현재 커진 지우개 크기

// 랜덤 색상 생성 (0.0f ~ 1.0f)
Color getRandomColor() {
	return {
		(float)rand() / (float)RAND_MAX,
		(float)rand() / (float)RAND_MAX,
		(float)rand() / (float)RAND_MAX
	};
}

// AABB (Axis-Aligned Bounding Box) 사각형 충돌 판정
bool checkCollision(float x1, float y1, float size1, float x2, float y2, float size2) {
	return (fabs(x1 - x2) < (size1 + size2)) && (fabs(y1 - y2) < (size1 + size2));
}

// 화면 초기화 및 20~40개 사각형 랜덤 생성
void initRectangles() {
	srand((unsigned int)time(NULL));
	initialRectCount = rand() % 21 + 20; // 20 ~ 40개 범위
	totalRectCount = initialRectCount;
	addedRectCount = 0;

	initialEraserHalfSize = RECT_HALF_SIZE * 2.0f;
	currentEraserHalfSize = initialEraserHalfSize;
	eraserColor = { 0.0f, 0.0f, 0.0f };

	for (int i = 0; i < totalRectCount; ++i) {
		// 화면 가장자리에 닿지 않도록 -0.9 ~ 0.9 범위로 제한
		rects[i].centerX = ((float)rand() / RAND_MAX) * 1.8f - 0.9f;
		rects[i].centerY = ((float)rand() / RAND_MAX) * 1.8f - 0.9f;
		rects[i].color = getRandomColor();
		rects[i].visible = true;
	}
}

// 마우스 좌표 (Screen Coordinate) -> OpenGL NDC 좌표 (-1.0 ~ 1.0) 변환
void getGLPos(GLFWwindow* window, double xpos, double ypos, float& glX, float& glY) {
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glX = (float)((xpos / width) * 2.0 - 1.0);
	glY = (float)(1.0 - (ypos / height) * 2.0);
}

// 지우개와 사각형들 간의 충돌 검사
void checkEraserCollisions() {
	if (!isEraserActive) return;

	for (int i = 0; i < totalRectCount; ++i) {
		if (rects[i].visible) {
			if (checkCollision(eraserX, eraserY, currentEraserHalfSize, rects[i].centerX, rects[i].centerY, RECT_HALF_SIZE)) {
				rects[i].visible = false;                   // 사각형 숨기기
				currentEraserHalfSize += 0.015f;            // 지우개 크기 확대
				eraserColor = rects[i].color;               // 부딪힌 사각형 색상으로 변경
			}
		}
	}
}

// 화면 렌더링
void DrawScene() {
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f); // 흰색 배경
	glClear(GL_COLOR_BUFFER_BIT);

	// 1. 일반 사각형 그리기
	for (int i = 0; i < totalRectCount; ++i) {
		if (rects[i].visible) {
			glColor3f(rects[i].color.r, rects[i].color.g, rects[i].color.b);
			glRectf(rects[i].centerX - RECT_HALF_SIZE, rects[i].centerY - RECT_HALF_SIZE,
				rects[i].centerX + RECT_HALF_SIZE, rects[i].centerY + RECT_HALF_SIZE);
		}
	}

	// 2. 지우개 사각형 그리기 (좌클릭 누르고 있을 때만)
	if (isEraserActive) {
		glColor3f(eraserColor.r, eraserColor.g, eraserColor.b);
		glRectf(eraserX - currentEraserHalfSize, eraserY - currentEraserHalfSize,
			eraserX + currentEraserHalfSize, eraserY + currentEraserHalfSize);
	}
}

// 마우스 클릭 이벤트 처리
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	float glX, glY;
	getGLPos(window, xpos, ypos, glX, glY);

	// 왼쪽 마우스 버튼 (지우개 활성화/비활성화)
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			isEraserActive = true;
			eraserX = glX;
			eraserY = glY;
			currentEraserHalfSize = initialEraserHalfSize; // 검정색/초기 크기로 리셋
			eraserColor = { 0.0f, 0.0f, 0.0f };
			checkEraserCollisions();
		}
		else if (action == GLFW_RELEASE) {
			isEraserActive = false;
			// 마우스를 떼면 지워졌던 사각형들이 원래 위치/색상으로 다시 나타남
			for (int i = 0; i < totalRectCount; ++i) {
				rects[i].visible = true;
			}
		}
	}

	// 오른쪽 마우스 버튼 (새 사각형 생성 - 최대 10개)
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		if (addedRectCount < 10 && totalRectCount < MAX_RECTS) {
			rects[totalRectCount].centerX = glX;
			rects[totalRectCount].centerY = glY;
			rects[totalRectCount].color = getRandomColor();
			rects[totalRectCount].visible = true;

			totalRectCount++;
			addedRectCount++;

			// 새 사각형이 생성되면 지우개 사각형의 기본 크기가 축소됨
			initialEraserHalfSize -= 0.005f;
			if (initialEraserHalfSize < RECT_HALF_SIZE) {
				initialEraserHalfSize = RECT_HALF_SIZE; // 최소 크기 제한
			}
		}
	}
}

// 마우스 커서 이동 이벤트 (드래그)
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	if (isEraserActive) {
		getGLPos(window, xpos, ypos, eraserX, eraserY);
		checkEraserCollisions();
	}
}

// 키보드 입력 처리
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action != GLFW_PRESS) return;

	// ESC 또는 Q: 종료
	if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
		glfwSetWindowShouldClose(window, true);
	}

	// r: 기존 사각형 삭제 후 리셋하여 다시 시작
	if (key == GLFW_KEY_R) {
		initRectangles();
	}
}

int main() {
	// GLFW 초기화
	if (!glfwInit()) {
		std::cerr << "GLFW 초기화 실패!" << std::endl;
		return -1;
	}

	// Compatibility Profile 설정
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	// 윈도우 생성
	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Practice 5 - Eraser", nullptr, nullptr);
	if (!window) {
		std::cerr << "윈도우 생성 실패!" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	// GLEW 초기화
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW 초기화 실패!" << std::endl;
		return -1;
	}

	// 사각형 데이터 생성
	initRectangles();

	// 콜백 함수 등록
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);

	glViewport(0, 0, 800, 600);

	// 메인 렌더링 루프
	while (!glfwWindowShouldClose(window)) {
		DrawScene();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}