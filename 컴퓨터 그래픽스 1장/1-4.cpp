#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// 색상 구조체
struct Color {
	float r, g, b;
};

// 사각형 구조체
struct Rect {
	float origX, origY; // 원래 생성 위치
	float centerX, centerY; // 현재 중심 위치
	float origR, origG, origB; // 원래 색상
	float r, g, b; // 현재 색상
	float vx, vy; // 속도
	float size; // 현재 사각형 반-크기 (Half size)
	float sizeDir; // 크기 변화 방향 (+/-)
	int clockState; // 시계방향 이동 상태 (0: top, 1: right, 2: bottom, 3: left)
	int zigzagDir; // 지그재그 이동 X 방향 (+1 / -1)
};

Rect rects[5];
int rectnum = 0;

// 키 토글 상태 플래그
bool press1 = false; // 대각선 이동
bool press2 = false; // 지그재그 이동
bool press3 = false; // 가장자리 시계방향 이동
bool press4 = false; // 크기 변화
bool press5 = false; // 색상 변화

double lastTime = 0.0f;
const float BASE_SIZE = 0.1f; // 기본 사각형 크기 (반지름/반너비 개념)
const float BASE_SPEED = 0.8f; // 이동 속도

// 랜덤 색상 반환 (0.0f ~ 1.0f)
Color getRandomColor() {
	return {
		(float)rand() / (float)RAND_MAX,
		(float)rand() / (float)RAND_MAX,
		(float)rand() / (float)RAND_MAX
	};
}

void DrawScene() {
	// 배경색: 짙은 회색
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	for (int i = 0; i < rectnum; ++i) {
		glColor3f(rects[i].r, rects[i].g, rects[i].b);
		// 사각형 중심 및 현재 크기를 기준으로 사각형 그리기
		glRectf(rects[i].centerX - rects[i].size, rects[i].centerY - rects[i].size,
			rects[i].centerX + rects[i].size, rects[i].centerY + rects[i].size);
	}
}

// 애니메이션 로직 업데이트
void updateAnimation() {
	double currentTime = glfwGetTime();
	float deltaTime = (float)(currentTime - lastTime);
	lastTime = currentTime;

	if (deltaTime <= 0.0f) return;

	for (int i = 0; i < rectnum; ++i) {
		// 1. 대각선 이동 및 벽 튕기기
		if (press1) {
			rects[i].centerX += rects[i].vx * deltaTime;
			rects[i].centerY += rects[i].vy * deltaTime;

			// 좌우 벽 충돌
			if (rects[i].centerX + rects[i].size >= 1.0f) {
				rects[i].centerX = 1.0f - rects[i].size;
				rects[i].vx *= -1.0f;
			}
			else if (rects[i].centerX - rects[i].size <= -1.0f) {
				rects[i].centerX = -1.0f + rects[i].size;
				rects[i].vx *= -1.0f;
			}

			// 상하 벽 충돌
			if (rects[i].centerY + rects[i].size >= 1.0f) {
				rects[i].centerY = 1.0f - rects[i].size;
				rects[i].vy *= -1.0f;
			}
			else if (rects[i].centerY - rects[i].size <= -1.0f) {
				rects[i].centerY = -1.0f + rects[i].size;
				rects[i].vy *= -1.0f;
			}
		}

		// 2. 가로 지그재그 이동
		if (press2) {
			float speed = BASE_SPEED * deltaTime;
			rects[i].centerX += rects[i].zigzagDir * speed;

			if (rects[i].centerX + rects[i].size >= 1.0f) {
				rects[i].centerX = 1.0f - rects[i].size;
				rects[i].zigzagDir = -1; // 왼쪽 방향 전환
				rects[i].centerY -= 0.1f; // 아래로 한 단계 이동
			}
			else if (rects[i].centerX - rects[i].size <= -1.0f) {
				rects[i].centerX = -1.0f + rects[i].size;
				rects[i].zigzagDir = 1; // 오른쪽 방향 전환
				rects[i].centerY -= 0.1f; // 아래로 한 단계 이동
			}

			// 화면 아래로 벗어날 경우 위로 되돌리기
			if (rects[i].centerY - rects[i].size < -1.0f) {
				rects[i].centerY = 1.0f - rects[i].size;
			}
		}

		// 3. 윈도우 가장자리를 따라 시계방향 이동
		if (press3) {
			float speed = BASE_SPEED * deltaTime;
			switch (rects[i].clockState) {
			case 0: // 위쪽 변에서 오른쪽으로 이동
				rects[i].centerX += speed;
				rects[i].centerY = 1.0f - rects[i].size;
				if (rects[i].centerX + rects[i].size >= 1.0f) {
					rects[i].centerX = 1.0f - rects[i].size;
					rects[i].clockState = 1;
				}
				break;
			case 1: // 오른쪽 변에서 아래로 이동
				rects[i].centerY -= speed;
				rects[i].centerX = 1.0f - rects[i].size;
				if (rects[i].centerY - rects[i].size <= -1.0f) {
					rects[i].centerY = -1.0f + rects[i].size;
					rects[i].clockState = 2;
				}
				break;
			case 2: // 아래쪽 변에서 왼쪽으로 이동
				rects[i].centerX -= speed;
				rects[i].centerY = -1.0f + rects[i].size;
				if (rects[i].centerX - rects[i].size <= -1.0f) {
					rects[i].centerX = -1.0f + rects[i].size;
					rects[i].clockState = 3;
				}
				break;
			case 3: // 왼쪽 변에서 위로 이동
				rects[i].centerY += speed;
				rects[i].centerX = -1.0f + rects[i].size;
				if (rects[i].centerY + rects[i].size >= 1.0f) {
					rects[i].centerY = 1.0f - rects[i].size;
					rects[i].clockState = 0;
				}
				break;
			}
		}

		// 4. 크기 변화
		if (press4) {
			rects[i].size += rects[i].sizeDir * deltaTime * 0.2f;
			if (rects[i].size > 0.25f) {
				rects[i].size = 0.25f;
				rects[i].sizeDir = -1.0f;
			}
			else if (rects[i].size < 0.03f) {
				rects[i].size = 0.03f;
				rects[i].sizeDir = 1.0f;
			}
		}

		// 5. 색상 랜덤 변화
		if (press5) {
			Color c = getRandomColor();
			rects[i].r = c.r;
			rects[i].g = c.g;
			rects[i].b = c.b;
		}
	}
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action != GLFW_PRESS) return;

	// 프로그램 종료 (q, ESC)
	if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) {
		glfwSetWindowShouldClose(window, true);
	}

	// 1: 대각선 이동 토글
	if (key == GLFW_KEY_1) press1 = !press1;

	// 2: 지그재그 이동 토글
	if (key == GLFW_KEY_2) press2 = !press2;

	// 3: 가장자리 시계방향 이동 토글
	if (key == GLFW_KEY_3) press3 = !press3;

	// 4: 크기 변화 토글
	if (key == GLFW_KEY_4) press4 = !press4;

	// 5: 색상 변화 토글
	if (key == GLFW_KEY_5) press5 = !press5;

	// s: 모든 애니메이션 멈춤
	if (key == GLFW_KEY_S) {
		press1 = press2 = press3 = press4 = press5 = false;
	}

	// m: 원래 그린 위치 및 크기로 복구
	if (key == GLFW_KEY_M) {
		for (int i = 0; i < rectnum; ++i) {
			rects[i].centerX = rects[i].origX;
			rects[i].centerY = rects[i].origY;
			rects[i].r = rects[i].origR;
			rects[i].g = rects[i].origG;
			rects[i].b = rects[i].origB;
			rects[i].size = BASE_SIZE;
			rects[i].clockState = 0;
			rects[i].zigzagDir = 1;
		}
	}

	// r: 사각형 삭제 및 리셋
	if (key == GLFW_KEY_R) {
		rectnum = 0;
		press1 = press2 = press3 = press4 = press5 = false;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		if (rectnum < 5) {
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);

			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			// 스크린 좌표 -> OpenGL NDC 좌표계 (-1.0 ~ 1.0) 변환
			float glX = (float)((xpos / width) * 2.0 - 1.0);
			float glY = (float)(1.0 - (ypos / height) * 2.0);

			Color c = getRandomColor();

			rects[rectnum].origX = rects[rectnum].centerX = glX;
			rects[rectnum].origY = rects[rectnum].centerY = glY;
			rects[rectnum].origR = rects[rectnum].r = c.r;
			rects[rectnum].origG = rects[rectnum].g = c.g;
			rects[rectnum].origB = rects[rectnum].b = c.b;

			// 대각선 이동 초기 속도 설정
			rects[rectnum].vx = (rectnum % 2 == 0) ? BASE_SPEED : -BASE_SPEED;
			rects[rectnum].vy = (rectnum % 3 == 0) ? BASE_SPEED : -BASE_SPEED;

			rects[rectnum].size = BASE_SIZE;
			rects[rectnum].sizeDir = 1.0f;
			rects[rectnum].clockState = 0;
			rects[rectnum].zigzagDir = 1;

			++rectnum;
		}
	}
}

int main() {
	srand((unsigned)time(NULL));

	// GLFW 초기화
	if (!glfwInit()) {
		std::cerr << "GLFW 초기화 실패!" << std::endl;
		return -1;
	}

	// Legacy / Compatibility 프로파일 설정
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	// 윈도우 생성
	GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Practice 4", nullptr, nullptr);
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

	// 콜백 함수 등록
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	glViewport(0, 0, 800, 600);
	lastTime = glfwGetTime();

	// 메인 루프
	while (!glfwWindowShouldClose(window)) {
		updateAnimation(); // 프레임 단위 데이터 업데이트
		DrawScene();       // 그리기

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// 종료 처리
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}