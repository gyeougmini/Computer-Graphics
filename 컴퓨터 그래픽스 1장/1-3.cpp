#include <GL/glew.h>
#include <GL/glfw3.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

const int WINDOW_WIDTH = 1020;
const int WINDOW_HEIGHT = 960;

struct Color {
    float r, g, b;
};

struct Rect {
    float x1, y1;
    float x2, y2;
    Color color;

    bool contains(float px, float py) const {
        return (px >= x1 && px <= x2 && py >= y1 && py <= y2);
    }

    bool overlaps(const Rect& other) const {
        return (x1 < other.x2 && x2 > other.x1 && y1 < other.y2 && y2 > other.y1);
    }
};

float getMin(float a, float b) { return (a < b) ? a : b; }
float getMax(float a, float b) { return (a > b) ? a : b; }
int getMinInt(int a, int b) { return (a < b) ? a : b; }
int getMaxInt(int a, int b) { return (a > b) ? a : b; }

float getRandomFloat(float minVal, float maxVal) {
    return minVal + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (maxVal - minVal)));
}

Color getRandomColor() {
    return { getRandomFloat(0.1f, 1.0f), getRandomFloat(0.1f, 1.0f), getRandomFloat(0.1f, 1.0f) };
}

std::vector<Rect> rects;
int selectedIndex = -1;
bool isDragging = false;
float dragOffsetX = 0.0f;
float dragOffsetY = 0.0f;

bool prevKeyAState = false;
bool prevRightMouseState = false;

void screenToOpenGLCoords(double screenX, double screenY, float& glX, float& glY) {
    glX = (float)((screenX / WINDOW_WIDTH) * 2.0 - 1.0);
    glY = (float)(1.0 - (screenY / WINDOW_HEIGHT) * 2.0);
}

void addRandomRect() {
    if (rects.size() >= 20) {
        std::cout << "최대 사각형 개수(20개)에 도달했습니다.\n";
        return;
    }

    float width = getRandomFloat(0.15f, 0.35f);
    float height = getRandomFloat(0.15f, 0.35f);

    float x1 = getRandomFloat(-0.9f, 0.9f - width);
    float y1 = getRandomFloat(-0.9f, 0.9f - height);

    Rect newRect;
    newRect.x1 = x1;
    newRect.y1 = y1;
    newRect.x2 = x1 + width;
    newRect.y2 = y1 + height;
    newRect.color = getRandomColor();

    rects.push_back(newRect);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    int keyAState = glfwGetKey(window, GLFW_KEY_A);
    if (keyAState == GLFW_PRESS && !prevKeyAState) {
        addRandomRect();
    }
    prevKeyAState = (keyAState == GLFW_PRESS);

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);
    float glX, glY;
    screenToOpenGLCoords(mouseX, mouseY, glX, glY);

    // 1. 왼쪽 마우스 버튼: 드래그 및 1.2배 확장 병합
    int leftButtonState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
    if (leftButtonState == GLFW_PRESS) {
        if (!isDragging) {
            for (int i = (int)rects.size() - 1; i >= 0; --i) {
                if (rects[i].contains(glX, glY)) {
                    selectedIndex = i;
                    isDragging = true;
                    dragOffsetX = glX - rects[i].x1;
                    dragOffsetY = glY - rects[i].y1;

                    Rect selectedRect = rects[i];
                    rects.erase(rects.begin() + i);
                    rects.push_back(selectedRect);
                    selectedIndex = (int)rects.size() - 1;
                    break;
                }
            }
        }
        else if (selectedIndex >= 0 && selectedIndex < (int)rects.size()) {
            float width = rects[selectedIndex].x2 - rects[selectedIndex].x1;
            float height = rects[selectedIndex].y2 - rects[selectedIndex].y1;

            rects[selectedIndex].x1 = glX - dragOffsetX;
            rects[selectedIndex].y1 = glY - dragOffsetY;
            rects[selectedIndex].x2 = rects[selectedIndex].x1 + width;
            rects[selectedIndex].y2 = rects[selectedIndex].y1 + height;
        }
    }
    else if (leftButtonState == GLFW_RELEASE && isDragging) {
        if (selectedIndex >= 0 && selectedIndex < (int)rects.size()) {
            Rect cur = rects[selectedIndex];

            for (int i = 0; i < (int)rects.size(); ++i) {
                if (i != selectedIndex && cur.overlaps(rects[i])) {
                    // 합칠 대상과의 바운딩 박스
                    float minX = getMin(cur.x1, rects[i].x1);
                    float minY = getMin(cur.y1, rects[i].y1);
                    float maxX = getMax(cur.x2, rects[i].x2);
                    float maxY = getMax(cur.y2, rects[i].y2);

                    float centerX = (minX + maxX) * 0.5f;
                    float centerY = (minY + maxY) * 0.5f;

                    // 바운딩 박스 기준으로 정확히 1.2배 확장
                    float newWidth = (maxX - minX) * 1.2f;
                    float newHeight = (maxY - minY) * 1.2f;

                    // 화면 경계 이탈 방지 상한선
                    if (newWidth > 1.8f) newWidth = 1.8f;
                    if (newHeight > 1.8f) newHeight = 1.8f;

                    Rect mergedRect;
                    mergedRect.x1 = centerX - newWidth * 0.5f;
                    mergedRect.y1 = centerY - newHeight * 0.5f;
                    mergedRect.x2 = centerX + newWidth * 0.5f;
                    mergedRect.y2 = centerY + newHeight * 0.5f;

                    // 화면(-1.0 ~ 1.0) 안으로 위치 보정
                    if (mergedRect.x1 < -1.0f) { mergedRect.x2 += (-1.0f - mergedRect.x1); mergedRect.x1 = -1.0f; }
                    if (mergedRect.x2 > 1.0f) { mergedRect.x1 -= (mergedRect.x2 - 1.0f);  mergedRect.x2 = 1.0f; }
                    if (mergedRect.y1 < -1.0f) { mergedRect.y2 += (-1.0f - mergedRect.y1); mergedRect.y1 = -1.0f; }
                    if (mergedRect.y2 > 1.0f) { mergedRect.y1 -= (mergedRect.y2 - 1.0f);  mergedRect.y2 = 1.0f; }

                    mergedRect.color = getRandomColor();

                    int firstIdx = getMaxInt(selectedIndex, i);
                    int secondIdx = getMinInt(selectedIndex, i);
                    rects.erase(rects.begin() + firstIdx);
                    rects.erase(rects.begin() + secondIdx);

                    rects.push_back(mergedRect);
                    break;
                }
            }
        }
        isDragging = false;
        selectedIndex = -1;
    }

    // 2. 오른쪽 마우스 버튼: 0.4배 ~ 최대 1.2배 범위로 2개 분할
    int rightButtonState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
    if (rightButtonState == GLFW_PRESS && !prevRightMouseState) {
        if (rects.size() < 20) {
            for (int i = (int)rects.size() - 1; i >= 0; --i) {
                if (rects[i].contains(glX, glY)) {
                    Rect target = rects[i];
                    rects.erase(rects.begin() + i);

                    float origW = target.x2 - target.x1;
                    float origH = target.y2 - target.y1;

                    for (int k = 0; k < 2; ++k) {
                        // 작아지거나(최소 0.4배) 커질 수 있도록(최대 1.2배) 설정
                        float scaleW = getRandomFloat(0.4f, 1.2f);
                        float scaleH = getRandomFloat(0.4f, 1.2f);

                        float w = origW * scaleW;
                        float h = origH * scaleH;

                        if (w < 0.05f) w = 0.05f;
                        if (h < 0.05f) h = 0.05f;
                        if (w > 1.5f)  w = 1.5f;
                        if (h > 1.5f)  h = 1.5f;

                        // 화면 전체 영역 내 랜덤 위치 배치
                        float x1 = getRandomFloat(-0.95f, 0.95f - w);
                        float y1 = getRandomFloat(-0.95f, 0.95f - h);

                        Rect piece;
                        piece.x1 = x1;
                        piece.y1 = y1;
                        piece.x2 = x1 + w;
                        piece.y2 = y1 + h;
                        piece.color = getRandomColor();

                        rects.push_back(piece);
                    }
                    break;
                }
            }
        }
    }
    prevRightMouseState = (rightButtonState == GLFW_PRESS);
}

void drawScene() {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    for (size_t i = 0; i < rects.size(); ++i) {
        glColor3f(rects[i].color.r, rects[i].color.g, rects[i].color.b);
        glRectf(rects[i].x1, rects[i].y1, rects[i].x2, rects[i].y2);
    }
}

int main() {
    srand((unsigned int)time(NULL));

    if (!glfwInit()) {
        std::cerr << "GLFW 초기화 실패!" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGL Practice 3", nullptr, nullptr);
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

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    while (!glfwWindowShouldClose(window)) {
        processInput(window);
        drawScene();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}