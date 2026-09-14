#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

// 색상 구조체
struct Color {
    float r, g, b;
};

// 랜덤 색상 반환 (0.0f ~ 1.0f)
Color getRandomColor() {
    return {
        (float)rand() / RAND_MAX,
        (float)rand() / RAND_MAX,
        (float)rand() / RAND_MAX
    };
}

// 사각형 구조체
struct Rect {
    int quadrant;       // 사분면 번호 (1, 2, 3, 4)
    float centerX;      // 중심 X 좌표
    float centerY;      // 중심 Y 좌표
    float size;         // 한 변의 절반 크기 (half-size)
    Color color;        // 색상
};

// 4개 분할 영역의 배경 색상 (인덱스 0~3: 1~4분면)
Color quadrantColors[4];

// 사각형 리스트
std::vector<Rect> rects;

// 현재 선택된 사각형 인덱스 (-1: 선택 없음)
int selectedIndex = -1;

// 각 사분면별 사각형 개수 확인 함수
int countRectsInQuadrant(int quad) {
    int count = 0;
    for (const auto& r : rects) {
        if (r.quadrant == quad) {
            count++;
        }
    }
    return count;
}

// 4개 분할 영역 색상 초기화/리셋
void initQuadrantColors() {
    for (int i = 0; i < 4; ++i) {
        quadrantColors[i] = getRandomColor();
    }
}

// 사각형 추가 함수 (영역당 최대 5개)
void addRect(int quad) {
    if (countRectsInQuadrant(quad) >= 5) {
        std::cout << quad << "사분면의 사각형 개수가 최대치(5개)에 도달했습니다." << std::endl;
        return;
    }

    // 각 사분면 중앙 좌표 설정
    float cx = 0.0f, cy = 0.0f;
    switch (quad) {
    case 1: cx = 0.5f;  cy = 0.5f;  break; // 우상단
    case 2: cx = -0.5f; cy = 0.5f;  break; // 좌상단
    case 3: cx = -0.5f; cy = -0.5f; break; // 좌하단
    case 4: cx = 0.5f;  cy = -0.5f; break; // 우하단
    }

    // 기본 랜덤 크기 (half-size: 0.1 ~ 0.35)
    float size = 0.1f + ((float)rand() / RAND_MAX) * 0.25f;

    Rect newRect = { quad, cx, cy, size, getRandomColor() };
    rects.push_back(newRect);
    selectedIndex = (int)rects.size() - 1; // 새로 생성된 사각형을 선택 상태로 설정
}

// 키보드 입력 콜백 함수
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    // 프로그램 종료 (q, ESC)
    if (key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, true);
    }
    // 1, 2, 3, 4번 사각형 생성
    else if (key == GLFW_KEY_1) {
        addRect(1);
    }
    else if (key == GLFW_KEY_2) {
        addRect(2);
    }
    else if (key == GLFW_KEY_3) {
        addRect(3);
    }
    else if (key == GLFW_KEY_4) {
        addRect(4);
    }
    // 선택된 사각형 크기 확대 (+)
    else if (key == GLFW_KEY_EQUAL || key == GLFW_KEY_KP_ADD) {
        if (selectedIndex >= 0 && selectedIndex < (int)rects.size()) {
            rects[selectedIndex].size += 0.02f;
            if (rects[selectedIndex].size > 0.45f) rects[selectedIndex].size = 0.45f;
        }
    }
    // 선택된 사각형 크기 축소 (-)
    else if (key == GLFW_KEY_MINUS || key == GLFW_KEY_KP_SUBTRACT) {
        if (selectedIndex >= 0 && selectedIndex < (int)rects.size()) {
            rects[selectedIndex].size -= 0.02f;
            if (rects[selectedIndex].size < 0.03f) rects[selectedIndex].size = 0.03f;
        }
    }
    // 선택된 사각형의 색상 랜덤 변경 (c)
    else if (key == GLFW_KEY_C) {
        if (selectedIndex >= 0 && selectedIndex < (int)rects.size()) {
            rects[selectedIndex].color = getRandomColor();
        }
    }
    // 리셋 (r): 사각형 모두 지우고 분할 배경색 재설정
    else if (key == GLFW_KEY_R) {
        rects.clear();
        selectedIndex = -1;
        initQuadrantColors();
    }
}

// 마우스 클릭 콜백 함수
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

        selectedIndex = -1;
        // 가장 최근에 그려진 사각형(맨 위)부터 충돌 검사
        for (int i = (int)rects.size() - 1; i >= 0; --i) {
            float x1 = rects[i].centerX - rects[i].size;
            float x2 = rects[i].centerX + rects[i].size;
            float y1 = rects[i].centerY - rects[i].size;
            float y2 = rects[i].centerY + rects[i].size;

            if (glX >= x1 && glX <= x2 && glY >= y1 && glY <= y2) {
                selectedIndex = i;
                break;
            }
        }
    }
}

// 화면 렌더링 함수
void DrawScene() {
    glClear(GL_COLOR_BUFFER_BIT);

    // 1. 화면 4분할 영역 그리기
    // 1사분면 (우상단: 0 ~ 1, 0 ~ 1)
    glColor3f(quadrantColors[0].r, quadrantColors[0].g, quadrantColors[0].b);
    glRectf(0.0f, 0.0f, 1.0f, 1.0f);

    // 2사분면 (좌상단: -1 ~ 0, 0 ~ 1)
    glColor3f(quadrantColors[1].r, quadrantColors[1].g, quadrantColors[1].b);
    glRectf(-1.0f, 0.0f, 0.0f, 1.0f);

    // 3사분면 (좌하단: -1 ~ 0, -1 ~ 0)
    glColor3f(quadrantColors[2].r, quadrantColors[2].g, quadrantColors[2].b);
    glRectf(-1.0f, -1.0f, 0.0f, 0.0f);

    // 4사분면 (우하단: 0 ~ 1, -1 ~ 0)
    glColor3f(quadrantColors[3].r, quadrantColors[3].g, quadrantColors[3].b);
    glRectf(0.0f, -1.0f, 1.0f, 0.0f);

    // 2. 생성된 사각형 그리기
    for (int i = 0; i < (int)rects.size(); ++i) {
        float x1 = rects[i].centerX - rects[i].size;
        float y1 = rects[i].centerY - rects[i].size;
        float x2 = rects[i].centerX + rects[i].size;
        float y2 = rects[i].centerY + rects[i].size;

        // 사각형 채우기
        glColor3f(rects[i].color.r, rects[i].color.g, rects[i].color.b);
        glRectf(x1, y1, x2, y2);

        // 3. 선택된 사각형일 경우 강조 테두리 그리기
        if (i == selectedIndex) {
            glColor3f(0.0f, 0.0f, 0.0f); // 검은색 테두리
            glLineWidth(3.0f);
            glBegin(GL_LINE_LOOP);
            glVertex2f(x1, y1);
            glVertex2f(x2, y1);
            glVertex2f(x2, y2);
            glVertex2f(x1, y2);
            glEnd();
        }
    }
}

int main() {
    srand((unsigned)time(NULL));

    // 4분할 영역 배경색 초기화
    initQuadrantColors();

    //--- GLFW 초기화
    if (!glfwInit()) {
        std::cerr << "GLFW 초기화 실패!" << std::endl;
        return -1;
    }

    //--- OpenGL 버전 및 프로파일 설정 (glRectf, glColor3f 사용을 위해 호환성 프로파일 설정)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    //--- 윈도우 생성
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Practice 2", nullptr, nullptr);
    if (!window) {
        std::cerr << "윈도우 생성 실패!" << std::endl;
        glfwTerminate();
        return -1;
    }

    //--- 컨텍스트 설정
    glfwMakeContextCurrent(window);

    //--- GLEW 초기화
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW 초기화 실패!" << std::endl;
        return -1;
    }

    // 콜백 함수 등록
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    //--- 뷰포트 설정
    glViewport(0, 0, 800, 600);

    //--- 메인 루프
    while (!glfwWindowShouldClose(window)) {
        DrawScene();

        // 버퍼 교체 및 이벤트 폴링
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //--- 종료 처리
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}