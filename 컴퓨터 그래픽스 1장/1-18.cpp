#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

GLuint shaderProgramID;

// OBJ 모델 데이터 구조체
typedef struct { float x, y, z; } Vertex;
typedef struct { unsigned int v1, v2, v3; } Face;
typedef struct {
    Vertex* vertices;
    size_t vertex_count;
    Face* faces;
    size_t face_count;
} Model;

GLuint sphereVAO, sphereVBO[2];
int sphereVertexCount = 0;

GLuint orbitVAO, orbitVBO;
const int ORBIT_SEGMENTS = 100;

// --- 상태 및 애니메이션 변수 ---
bool isPerspective = true; // p/P: 투영
bool isSolid = true;       // m/M: 와이어/솔리드

glm::vec3 globalOffset = glm::vec3(0.0f); // w/a/s/d/+/- 객체 이동
float orbitRadiusScale = 1.0f;           // 1: 궤도 반지름 조절

// 2: 중심 구 애니메이션 관련
bool animCenter = false;
float centerScale = 1.0f;
float centerScaleDir = 1.0f;
glm::vec3 centerColor = glm::vec3(0.0f, 0.0f, 1.0f); // 파란색 기본

// 3: 달 궤도 자전 애니메이션 관련
bool animMoonOrbit = false;
float moonOrbitRotZ = 0.0f;

// 공전 각도 변수
float planetOrbitAngle[3] = { 0.0f, 0.0f, 0.0f };
float moonOrbitAngle[3] = { 0.0f, 0.0f, 0.0f };

void read_newline(char* str) {
    char* pos;
    if ((pos = strchr(str, '\n')) != NULL) *pos = '\0';
}

void read_obj_file(const char* filename, Model* model) {
    FILE* file = NULL;
    fopen_s(&file, filename, "r");
    if (!file) {
        std::cout << "OBJ 파일을 열 수 없습니다: " << filename << std::endl;
        model->vertex_count = 0;
        model->face_count = 0;
        return;
    }

    char line[512];
    model->vertex_count = 0;
    model->face_count = 0;

    while (fgets(line, sizeof(line), file)) {
        read_newline(line);
        if (line[0] == 'v' && line[1] == ' ') model->vertex_count++;
        else if (line[0] == 'f' && line[1] == ' ') model->face_count++;
    }

    fseek(file, 0, SEEK_SET);

    model->vertices = (Vertex*)malloc(model->vertex_count * sizeof(Vertex));
    model->faces = (Face*)malloc(model->face_count * sizeof(Face));

    size_t vertex_index = 0, face_index = 0;

    while (fgets(line, sizeof(line), file)) {
        read_newline(line);
        if (line[0] == 'v' && line[1] == ' ') {
            sscanf_s(line + 2, "%f %f %f",
                &model->vertices[vertex_index].x,
                &model->vertices[vertex_index].y,
                &model->vertices[vertex_index].z);
            vertex_index++;
        }
        else if (line[0] == 'f' && line[1] == ' ') {
            unsigned int v1, v2, v3;
            if (strstr(line, "/")) {
                sscanf_s(line + 2, "%u/%*s %u/%*s %u/%*s", &v1, &v2, &v3);
            }
            else {
                sscanf_s(line + 2, "%u %u %u", &v1, &v2, &v3);
            }
            model->faces[face_index].v1 = v1 - 1;
            model->faces[face_index].v2 = v2 - 1;
            model->faces[face_index].v3 = v3 - 1;
            face_index++;
        }
    }
    fclose(file);
}

std::string filetobuf(const char* file) {
    std::ifstream shaderFile(file);
    if (!shaderFile.is_open()) return "";
    return std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
}

void InitShader() {
    std::string vStr = filetobuf("vertex16.glsl");
    std::string fStr = filetobuf("fragment16.glsl");
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

void InitBuffers() {
    Model mySphereModel;
    read_obj_file("sphere18.obj", &mySphereModel);
    if (mySphereModel.face_count > 0) {
        sphereVertexCount = static_cast<int>(mySphereModel.face_count * 3);
        std::vector<GLfloat> objShape(sphereVertexCount * 3);
        std::vector<GLfloat> objColors(sphereVertexCount * 3);

        for (size_t i = 0; i < mySphereModel.face_count; i++) {
            objShape[i * 9 + 0] = mySphereModel.vertices[mySphereModel.faces[i].v1].x;
            objShape[i * 9 + 1] = mySphereModel.vertices[mySphereModel.faces[i].v1].y;
            objShape[i * 9 + 2] = mySphereModel.vertices[mySphereModel.faces[i].v1].z;
            objShape[i * 9 + 3] = mySphereModel.vertices[mySphereModel.faces[i].v2].x;
            objShape[i * 9 + 4] = mySphereModel.vertices[mySphereModel.faces[i].v2].y;
            objShape[i * 9 + 5] = mySphereModel.vertices[mySphereModel.faces[i].v2].z;
            objShape[i * 9 + 6] = mySphereModel.vertices[mySphereModel.faces[i].v3].x;
            objShape[i * 9 + 7] = mySphereModel.vertices[mySphereModel.faces[i].v3].y;
            objShape[i * 9 + 8] = mySphereModel.vertices[mySphereModel.faces[i].v3].z;

            for (int k = 0; k < 3; k++) {
                objColors[i * 9 + k * 3 + 0] = 1.0f;
                objColors[i * 9 + k * 3 + 1] = 1.0f;
                objColors[i * 9 + k * 3 + 2] = 1.0f;
            }
        }

        glGenVertexArrays(1, &sphereVAO);
        glBindVertexArray(sphereVAO);
        glGenBuffers(2, sphereVBO);

        glBindBuffer(GL_ARRAY_BUFFER, sphereVBO[0]);
        glBufferData(GL_ARRAY_BUFFER, objShape.size() * sizeof(GLfloat), objShape.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, sphereVBO[1]);
        glBufferData(GL_ARRAY_BUFFER, objColors.size() * sizeof(GLfloat), objColors.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        free(mySphereModel.vertices);
        free(mySphereModel.faces);
    }

    std::vector<GLfloat> circleVertices;
    for (int i = 0; i < ORBIT_SEGMENTS; ++i) {
        float theta = 2.0f * M_PI * float(i) / float(ORBIT_SEGMENTS);
        circleVertices.push_back(cosf(theta));
        circleVertices.push_back(0.0f);
        circleVertices.push_back(sinf(theta));
    }

    glGenVertexArrays(1, &orbitVAO);
    glBindVertexArray(orbitVAO);
    glGenBuffers(1, &orbitVBO);

    glBindBuffer(GL_ARRAY_BUFFER, orbitVBO);
    glBufferData(GL_ARRAY_BUFFER, circleVertices.size() * sizeof(GLfloat), circleVertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    bool isShift = (mods & GLFW_MOD_SHIFT) != 0;

    switch (key) {
    case GLFW_KEY_Q:
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

    case GLFW_KEY_P:
        isPerspective = !isPerspective;
        break;

    case GLFW_KEY_M:
        isSolid = !isSolid;
        break;

    case GLFW_KEY_W: globalOffset.y += 0.1f; break;
    case GLFW_KEY_S: globalOffset.y -= 0.1f; break;
    case GLFW_KEY_A: globalOffset.x -= 0.1f; break;
    case GLFW_KEY_D: globalOffset.x += 0.1f; break;

    case GLFW_KEY_EQUAL:
        globalOffset.z += 0.1f;
        break;
    case GLFW_KEY_MINUS:
        globalOffset.z -= 0.1f;
        break;

    case GLFW_KEY_1:
        if (isShift) orbitRadiusScale -= 0.1f;
        else orbitRadiusScale += 0.1f;
        if (orbitRadiusScale < 0.2f) orbitRadiusScale = 0.2f;
        break;

    case GLFW_KEY_2:
        animCenter = !animCenter;
        break;

    case GLFW_KEY_3:
        animMoonOrbit = !animMoonOrbit;
        break;
    }
}

void UpdateAnimations(float deltaTime) {
    float planetSpeeds[3] = { 40.0f, 60.0f, 30.0f };
    float moonSpeeds[3] = { 120.0f, 150.0f, 100.0f };

    for (int i = 0; i < 3; ++i) {
        planetOrbitAngle[i] += planetSpeeds[i] * deltaTime;
        if (planetOrbitAngle[i] >= 360.0f) planetOrbitAngle[i] -= 360.0f;

        moonOrbitAngle[i] += moonSpeeds[i] * deltaTime;
        if (moonOrbitAngle[i] >= 360.0f) moonOrbitAngle[i] -= 360.0f;
    }

    if (animCenter) {
        centerScale += centerScaleDir * deltaTime * 0.5f;
        if (centerScale > 1.5f) { centerScale = 1.5f; centerScaleDir = -1.0f; }
        if (centerScale < 0.6f) { centerScale = 0.6f; centerScaleDir = 1.0f; }

        float t = (centerScale - 0.6f) / 0.9f;
        centerColor = glm::vec3(t, 0.2f, 1.0f - t);
    }

    if (animMoonOrbit) {
        moonOrbitRotZ += 90.0f * deltaTime;
        if (moonOrbitRotZ >= 360.0f) moonOrbitRotZ -= 360.0f;
    }
}

void renderSphere(const glm::mat4& model, const glm::mat4& viewProj, GLuint transformLoc, glm::vec3 color) {
    glm::mat4 mvp = viewProj * model;
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    std::vector<GLfloat> colors(sphereVertexCount * 3);
    for (int i = 0; i < sphereVertexCount; ++i) {
        colors[i * 3 + 0] = color.r;
        colors[i * 3 + 1] = color.g;
        colors[i * 3 + 2] = color.b;
    }

    glBindVertexArray(sphereVAO);
    glBindBuffer(GL_ARRAY_BUFFER, sphereVBO[1]);
    glBufferSubData(GL_ARRAY_BUFFER, 0, colors.size() * sizeof(GLfloat), colors.data());

    glDrawArrays(GL_TRIANGLES, 0, sphereVertexCount);
}

void renderOrbit(const glm::mat4& model, const glm::mat4& viewProj, GLuint transformLoc, glm::vec3 color) {
    glm::mat4 mvp = viewProj * model;
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mvp));

    std::vector<GLfloat> colors(ORBIT_SEGMENTS * 3);
    for (int i = 0; i < ORBIT_SEGMENTS; ++i) {
        colors[i * 3 + 0] = color.r;
        colors[i * 3 + 1] = color.g;
        colors[i * 3 + 2] = color.b;
    }

    glBindVertexArray(orbitVAO);
    glLineWidth(1.2f);
    glDrawArrays(GL_LINE_LOOP, 0, ORBIT_SEGMENTS);
}

void drawScene(GLFWwindow* window, float deltaTime) {
    UpdateAnimations(deltaTime);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (isSolid) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    else glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    glUseProgram(shaderProgramID);
    GLuint transformLoc = glGetUniformLocation(shaderProgramID, "transform");

    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 3.0f, 6.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 projection;
    if (isPerspective) {
        projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    }
    else {
        projection = glm::ortho(-3.0f, 3.0f, -3.0f, 3.0f, 0.1f, 100.0f);
    }

    glm::mat4 viewProj = projection * view;

    glm::mat4 masterModel = glm::mat4(1.0f);
    masterModel = glm::translate(masterModel, globalOffset);

    // --- 1. 중심 구 (태양/핵): 처음 크기(0.3 * 0.01)의 정확히 2배 ---
    glm::mat4 centerModel = masterModel;
    centerModel = glm::scale(centerModel, glm::vec3(0.3f * centerScale));
    centerModel = glm::scale(centerModel, glm::vec3(0.03f)); // 0.01f -> 0.02f (2배)
    renderSphere(centerModel, viewProj, transformLoc, centerColor);

    // --- 3개 행성 및 궤도 구성 데이터 ---
    float baseOrbitRadii[3] = { 1.5f, 2.1f, 2.7f };
    float orbitTiltAngles[3] = { 0.0f, 45.0f, -45.0f };
    glm::vec3 planetColors[3] = {
        glm::vec3(0.1f, 0.8f, 0.3f),
        glm::vec3(0.9f, 0.5f, 0.1f),
        glm::vec3(0.2f, 0.6f, 0.9f)
    };

    float moonOrbitRadius = 0.4f;

    for (int i = 0; i < 3; ++i) {
        float r = baseOrbitRadii[i] * orbitRadiusScale;

        // 행성 공전 궤도선
        glm::mat4 planetOrbitModel = masterModel;
        planetOrbitModel = glm::rotate(planetOrbitModel, glm::radians(orbitTiltAngles[i]), glm::vec3(0.0f, 0.0f, 1.0f));
        planetOrbitModel = glm::scale(planetOrbitModel, glm::vec3(r, 1.0f, r));
        renderOrbit(planetOrbitModel, viewProj, transformLoc, glm::vec3(0.4f));

        // --- 행성: 처음 크기(0.15 * 0.01)의 정확히 2배 ---
        glm::mat4 planetModel = masterModel;
        planetModel = glm::rotate(planetModel, glm::radians(orbitTiltAngles[i]), glm::vec3(0.0f, 0.0f, 1.0f));
        planetModel = glm::rotate(planetModel, glm::radians(planetOrbitAngle[i]), glm::vec3(0.0f, 1.0f, 0.0f));
        planetModel = glm::translate(planetModel, glm::vec3(r, 0.0f, 0.0f));

        glm::mat4 planetPosModel = planetModel;

        planetModel = glm::scale(planetModel, glm::vec3(0.15f));
        planetModel = glm::scale(planetModel, glm::vec3(0.03f)); // 0.01f -> 0.02f (2배)
        renderSphere(planetModel, viewProj, transformLoc, planetColors[i]);

        // 달 궤도선
        glm::mat4 moonOrbitModel = planetPosModel;
        moonOrbitModel = glm::rotate(moonOrbitModel, glm::radians(moonOrbitRotZ), glm::vec3(0.0f, 0.0f, 1.0f));

        glm::mat4 moonOrbitLine = glm::scale(moonOrbitModel, glm::vec3(moonOrbitRadius, 1.0f, moonOrbitRadius));
        renderOrbit(moonOrbitLine, viewProj, transformLoc, glm::vec3(0.6f, 0.6f, 0.2f));

        // --- 달: 처음 크기(0.07 * 0.01)의 정확히 2배 ---
        glm::mat4 moonModel = moonOrbitModel;
        moonModel = glm::rotate(moonModel, glm::radians(moonOrbitAngle[i]), glm::vec3(0.0f, 1.0f, 0.0f));
        moonModel = glm::translate(moonModel, glm::vec3(moonOrbitRadius, 0.0f, 0.0f));
        moonModel = glm::scale(moonModel, glm::vec3(0.07f));
        moonModel = glm::scale(moonModel, glm::vec3(0.03f)); // 0.01f -> 0.02f (2배)

        renderSphere(moonModel, viewProj, transformLoc, glm::vec3(1.0f, 0.2f, 0.2f));
    }
}

int main() {
    srand(static_cast<unsigned int>(time(NULL)));

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Practice 18", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    InitShader();
    InitBuffers();

    glfwSetKeyCallback(window, key_callback);

    float lastTime = static_cast<float>(glfwGetTime());

    while (!glfwWindowShouldClose(window)) {
        float currentTime = static_cast<float>(glfwGetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        if (deltaTime > 0.1f) deltaTime = 0.1f;

        drawScene(window, deltaTime);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}