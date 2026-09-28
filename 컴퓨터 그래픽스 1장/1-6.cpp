#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define MAX_RECTS 200

// 색상 구조체
struct Color {
	float r, g, b;
};

// 사각형 구조체
struct Rect {
	float centerX, centerY; // 중심 좌표
	float size;             // 반 크기 (Half Size)
	Color color;            // 현재 색상
	bool active;            // 활성화 여부
	bool isFragment;        // 클릭되어 분할된 조각인지 여부

	// 조각 애니메이션 변수
	float vx, vy;           // 이동 속도 및 방향
	int colorFadeDir;       // 색상 변화 방향 (+1: 밝아짐, -1: 어두워짐)
};

Rect rects[MAX_RECTS];
int rectCount = 0;
double lastTime = 0.0f;

// 0.0f ~ 1.0f 임의의 float 반환
float getRandomFloat(float min, float max) {
	return min + ((float)rand() / (float)RAND_MAX) * (max - min);
}

// 랜덤 색상 반환
Color getRandomColor() {
	return { getRandomFloat(0.2f, 0.9f), getRandomFloat(0.2f, 0.9f), getRandomFloat(0.2f, 0.9f) };
}

// 점이 사각형 내부인지 판정 (Point in AABB)
bool isPointInsideRect(float px, float py, const Rect& r) {
	return (px >= r.centerX - r.size && px <= r.centerX + r.size &&
		py >= r.centerY - r.size && py <= r.centerY + r.size);
}

// 초기 5~10개 사각형 생성
void initScene() {
	srand((unsigned int)time(NULL));
	rectCount = rand() % 6 + 5; // 5 ~ 10개

	for (int i = 0; i < rectCount; ++i) {
		rects[i].centerX = getRandomFloat(-0.7f, 0.7f);
		rects[i].centerY = getRandomFloat(-0.7f, 0.7f);
		rects[i].size = getRandomFloat(0.08f, 0.15f);
		rects[i].color = getRandomColor();
		rects[i].active = true;
		rects[i].isFragment = false;
		rects[i].vx = 0.0f;
		rects[i].vy = 0.0f;
	}
}

// 조각 사각형 생성 함수
void createFragment(float x, float y, float size, Color col, float vx, float vy, int fadeDir) {
	if (rectCount >= MAX_RECTS) return;

	rects[rectCount].centerX = x;
	rects[rectCount].centerY = y;
	rects[rectCount].size = size;
	rects[rectCount].color = col;
	rects[rectCount].active = true;
	rects[rectCount].isFragment = true;
	rects[rectCount].vx = vx;
	rects[rectCount].vy = vy;
	rects[rectCount].colorFadeDir = fadeDir;

	rectCount++;
}

// 사각형 분할 및 애니메이션 설정
void splitRectangle(int targetIdx) {
	Rect parent = rects[targetIdx];
	rects[targetIdx].active = false; // 부모 사각형 제거

	int pattern = rand() % 4; // 0: 좌우상하, 1: 대각선, 2: 한쪽방향, 3: 8방향
	float fragSize = parent.size * 0.4f;
	float speed = 0.6f;
	int fadeDir = (rand() % 2 == 0) ? 1 : -1; // 1: 밝아짐, -1: 어두워짐

	if (pattern == 0) { // ① 좌우상하 4방향
		createFragment(parent.centerX, parent.centerY + fragSize, fragSize, parent.color, 0.0f, speed, fadeDir);
		createFragment(parent.centerX, parent.centerY - fragSize, fragSize, parent.color, 0.0f, -speed, fadeDir);
		createFragment(parent.centerX - fragSize, parent.centerY, fragSize, parent.color, -speed, 0.0f, fadeDir);
		createFragment(parent.centerX + fragSize, parent.centerY, fragSize, parent.color, speed, 0.0f, fadeDir);
	}
	else if (pattern == 1) { // ② 대각선 4방향
		createFragment(parent.centerX - fragSize, parent.centerY + fragSize, fragSize, parent.color, -speed, speed, fadeDir);
		createFragment(parent.centerX + fragSize, parent.centerY + fragSize, fragSize, parent.color, speed, speed, fadeDir);
		createFragment(parent.centerX - fragSize, parent.centerY - fragSize, fragSize, parent.color, -speed, -speed, fadeDir);
		createFragment(parent.centerX + fragSize, parent.centerY - fragSize, fragSize, parent.color, speed, -speed, fadeDir);
	}
	else if (pattern == 2) { // ③ 한쪽 방향 같이 이동
		float angle = getRandomFloat(0.0f, 6.28f);
		float vx = cos(angle) * speed;
		float vy = sin(angle) * speed;

		createFragment(parent.centerX - fragSize, parent.centerY + fragSize, fragSize, parent.color, vx, vy, fadeDir);
		createFragment(parent.centerX + fragSize, parent.centerY + fragSize, fragSize, parent.color, vx, vy, fadeDir);
		createFragment(parent.centerX - fragSize, parent.centerY - fragSize, fragSize, parent.color, vx, vy, fadeDir);
		createFragment(parent.centerX + fragSize, parent.centerY - fragSize, fragSize, parent.color, vx, vy, fadeDir);
	}
	else if (pattern == 3) { // ④ 8방향 이동 (8개 조각 생성)
		float dirX[8] = { 0,  0, -1, 1, -1,  1, -1, 1 };
		float dirY[8] = { 1, -1,  0, 0,  1,  1, -1, -1 };

		for (int i = 0; i < 8; ++i) {
			createFragment(parent.centerX, parent.centerY, fragSize * 0.8f, parent.color, dirX[i] * speed, dirY[i] * speed, fadeDir);
		}
	}
}

// 매 프레임 애니메이션 및 상태 업데이트
void updateAnimation() {
	double currentTime = glfwGetTime();
	float deltaTime = (float)(currentTime - lastTime);
	lastTime = currentTime;

	if (deltaTime <= 0.0f) return;

	for (int i = 0; i < rectCount; ++i) {
		if (!rects[i].active || !rects[i].isFragment) continue;

		// 1. 위치 이동
		rects[i].centerX += rects[i].vx * deltaTime;
		rects[i].centerY += rects[i].vy * deltaTime;

		// 2. 크기 축소 및 소멸 처리
		rects[i].size -= deltaTime * 0.05f;
		if (rects[i].size <= 0.01f) {
			rects[i].active = false;
			continue;
		}

		// 3. 색상 밝기/어두움 변화
		float fadeRate = deltaTime * 0.8f * rects[i].colorFadeDir;
		rects[i].color.r += fadeRate;
		rects[i].color.g += fadeRate;
		rects[i].color.b += fadeRate;

		// RGB 범위를 0.0 ~ 1.0 사이로 클램핑
		if (rects[i].color.r > 1.0f) rects[i].color.r = 1.0f;
		if (rects[i].color.g > 1.0f) rects[i].color.g = 1.0f;
		if (rects[i].color.b > 1.0f) rects[i].color.b = 1.0f;
		if (rects[i].color.r < 0.0f) rects[i].color.r = 0.0f;
		if (rects[i].color.g < 0.0f) rects[i].color.g = 0.0f;
		if (rects[i].color.b < 0.0f) rects[i].color.b = 0.0f;
	}
}

// 장면 렌더링
void DrawScene() {
	glClearColor(0.15f, 0.15f, 0.15f, 1.0f); // 배경색: 짙은 회색
	glClear(GL_COLOR_BUFFER_BIT);

	for (int i = 0; i < rectCount; ++i) {
		if (rects[i].active) {
			glColor3f(rects[i].color.r, rects[i].color.g, rects[i].color.b);
			glRectf(rects[i].centerX - rects[i].size, rects[i].centerY - rects[i].size,
				rects[i].centerX + rects[i].size, rects[i].centerY + rects[i].size);
		}
	}
}

// 마우스 클릭 이벤트 Callback
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);

		int width, height;
		glfwGetFramebufferSize(window, &width, &height);

		// 스크린 좌표 -> NDC 좌표 (-1.0 ~ 1.0) 변환
		float glX = (float)((xpos / width) * 2.0 - 1.0);
		float glY = (float)(1.0 - (ypos / height) * 2.0);

		// 클릭 위치에 있는 활성화된 사각형 탐색 (최상단부터)
		for (int i = rectCount - 1; i >= 0; --i) {
			if (rects[i].active && isPointInsideRect(glX, glY, rects[i])) {
				splitRectangle(i);
				break;
			}
		}
	}
}

// 키보드 Callback
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action != GLFW_PRESS) return;

	if (key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) {
		glfwSetWindowShouldClose(window, true);
	}

	// r: 리셋 및 사각형 재생성
	if (key == GLFW_KEY_R) {
		initScene();
	}
}

int main() {
	if (!glfwInit()) {
		std::cerr << "GLFW 초기화 실패!" << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Practice 6 - Explosion", nullptr, nullptr);
	if (!window) {
		std::cerr << "윈도우 생성 실패!" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);

	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		std::cerr << "GLEW 초기화 실패!" << std::endl;
		return -1;
	}

	initScene();

	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glViewport(0, 0, 800, 600);
	lastTime = glfwGetTime();

	while (!glfwWindowShouldClose(window)) {
		updateAnimation();
		DrawScene();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}