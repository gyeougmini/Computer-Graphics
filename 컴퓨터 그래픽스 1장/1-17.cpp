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

GLuint shaderProgramID;
GLuint axisvao, axisvbo[2];

// 선택 상태 (1: 좌측, 2: 우측, 3: 둘 다)
bool target1 = true;
bool target2 = true;

bool shapeSet = false; // c키 토글


struct TransformState {
    // 자전 / 공전 각도
    float rotX = 0.0f;
    float rotY = 0.0f;
    float orbitY = 0.0f;

    // 회전 애니메이션 속도 및 방향 (0.0: 정지, 1.0: 양, -1.0: 음)
    float rotXDir = 0.0f;
    float rotYDir = 0.0f;
    float orbitYDir = 0.0f;

    // 스케일 및 이동
    float scaleLocal = 1.0f;  // 제자리 스케일 (a/A)
    float scaleGlobal = 1.0f; // 원점 스케일 (b/B)
    glm::vec3 pos = glm::vec3(0.0f); // 이동 위치 (d/D, e/E)
};

// Index 0: 좌측 객체, Index 1: 우측 객체
TransformState objState[2];

// 기본 위치 offset (좌/우)
const float BASE_OFFSET = 1.0f;

// --- 이동 및 특수 애니메이션 변수 ---
bool animT = false; float tProgress = 0.0f; float tDir = 1.0f;
bool animU = false; float uProgress = 0.0f; float uDir = 1.0f;
bool animV = false; float vAngle = 0.0f;

// --- OBJ 관련 전역 변수 ---
GLuint objspherevao, objspherevbo[2];
GLuint objconevao, objconevbo[2];
GLuint objsquarevao, objsquarevbo[2];
GLuint objcylindervao, objcylindervbo[2];

int objSphereVertexCount = 0;
int objConeVertexCount = 0;
int objSquareVertexCount = 0;
int objCylinderVertexCount = 0;

typedef struct { float x, y, z; } Vertex;
typedef struct { unsigned int v1, v2, v3; } Face;
typedef struct {
    Vertex* vertices;
    size_t vertex_count;
    Face* faces;
    size_t face_count;
} Model;

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
    std::string vStr = filetobuf("vertex17.glsl");
    std::string fStr = filetobuf("fragment17.glsl");
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

void InitBuffer() {
    GLfloat axisShape[6][3] = {
        {-2.0f, 0.0f, 0.0f}, { 2.0f, 0.0f, 0.0f},
        { 0.0f,-2.0f, 0.0f}, { 0.0f, 2.0f, 0.0f},
        { 0.0f, 0.0f,-2.0f}, { 0.0f, 0.0f, 2.0f}
    };

    GLfloat axisColors[6][3] = {
        {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
        {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
        {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}
    };

    glGenVertexArrays(1, &axisvao);
    glBindVertexArray(axisvao);
    glGenBuffers(2, axisvbo);

    glBindBuffer(GL_ARRAY_BUFFER, axisvbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisShape), axisShape, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, axisvbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisColors), axisColors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    // Sphere OBJ
    Model mySphereModel;
    read_obj_file("sphere.obj", &mySphereModel);
    if (mySphereModel.face_count > 0) {
        objSphereVertexCount = static_cast<int>(mySphereModel.face_count * 3);
        std::vector<GLfloat> objShape(objSphereVertexCount * 3);
        std::vector<GLfloat> objColors(objSphereVertexCount * 3);

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
                objColors[i * 9 + k * 3 + 0] = 0.9f;
                objColors[i * 9 + k * 3 + 1] = 0.5f;
                objColors[i * 9 + k * 3 + 2] = 0.2f;
            }
        }

        glGenVertexArrays(1, &objspherevao);
        glBindVertexArray(objspherevao);
        glGenBuffers(2, objspherevbo);

        glBindBuffer(GL_ARRAY_BUFFER, objspherevbo[0]);
        glBufferData(GL_ARRAY_BUFFER, objShape.size() * sizeof(GLfloat), objShape.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, objspherevbo[1]);
        glBufferData(GL_ARRAY_BUFFER, objColors.size() * sizeof(GLfloat), objColors.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        free(mySphereModel.vertices);
        free(mySphereModel.faces);
    }

    // Cone OBJ
    Model myConeModel;
    read_obj_file("cone.obj", &myConeModel);
    if (myConeModel.face_count > 0) {
        objConeVertexCount = static_cast<int>(myConeModel.face_count * 3);
        std::vector<GLfloat> objShape(objConeVertexCount * 3);
        std::vector<GLfloat> objColors(objConeVertexCount * 3);

        for (size_t i = 0; i < myConeModel.face_count; i++) {
            objShape[i * 9 + 0] = myConeModel.vertices[myConeModel.faces[i].v1].x;
            objShape[i * 9 + 1] = myConeModel.vertices[myConeModel.faces[i].v1].y;
            objShape[i * 9 + 2] = myConeModel.vertices[myConeModel.faces[i].v1].z;
            objShape[i * 9 + 3] = myConeModel.vertices[myConeModel.faces[i].v2].x;
            objShape[i * 9 + 4] = myConeModel.vertices[myConeModel.faces[i].v2].y;
            objShape[i * 9 + 5] = myConeModel.vertices[myConeModel.faces[i].v2].z;
            objShape[i * 9 + 6] = myConeModel.vertices[myConeModel.faces[i].v3].x;
            objShape[i * 9 + 7] = myConeModel.vertices[myConeModel.faces[i].v3].y;
            objShape[i * 9 + 8] = myConeModel.vertices[myConeModel.faces[i].v3].z;

            for (int k = 0; k < 3; k++) {
                objColors[i * 9 + k * 3 + 0] = 0.2f;
                objColors[i * 9 + k * 3 + 1] = 0.8f;
                objColors[i * 9 + k * 3 + 2] = 0.4f;
            }
        }

        glGenVertexArrays(1, &objconevao);
        glBindVertexArray(objconevao);
        glGenBuffers(2, objconevbo);

        glBindBuffer(GL_ARRAY_BUFFER, objconevbo[0]);
        glBufferData(GL_ARRAY_BUFFER, objShape.size() * sizeof(GLfloat), objShape.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, objconevbo[1]);
        glBufferData(GL_ARRAY_BUFFER, objColors.size() * sizeof(GLfloat), objColors.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        free(myConeModel.vertices);
        free(myConeModel.faces);
    }

    // Square OBJ
    Model mySquareModel;
    read_obj_file("square.obj", &mySquareModel);
    if (mySquareModel.face_count > 0) {
        objSquareVertexCount = static_cast<int>(mySquareModel.face_count * 3);
        std::vector<GLfloat> objShape(objSquareVertexCount * 3);
        std::vector<GLfloat> objColors(objSquareVertexCount * 3);

        for (size_t i = 0; i < mySquareModel.face_count; i++) {
            objShape[i * 9 + 0] = mySquareModel.vertices[mySquareModel.faces[i].v1].x;
            objShape[i * 9 + 1] = mySquareModel.vertices[mySquareModel.faces[i].v1].y;
            objShape[i * 9 + 2] = mySquareModel.vertices[mySquareModel.faces[i].v1].z;
            objShape[i * 9 + 3] = mySquareModel.vertices[mySquareModel.faces[i].v2].x;
            objShape[i * 9 + 4] = mySquareModel.vertices[mySquareModel.faces[i].v2].y;
            objShape[i * 9 + 5] = mySquareModel.vertices[mySquareModel.faces[i].v2].z;
            objShape[i * 9 + 6] = mySquareModel.vertices[mySquareModel.faces[i].v3].x;
            objShape[i * 9 + 7] = mySquareModel.vertices[mySquareModel.faces[i].v3].y;
            objShape[i * 9 + 8] = mySquareModel.vertices[mySquareModel.faces[i].v3].z;

            for (int k = 0; k < 3; k++) {
                objColors[i * 9 + k * 3 + 0] = 0.3f;
                objColors[i * 9 + k * 3 + 1] = 0.6f;
                objColors[i * 9 + k * 3 + 2] = 0.9f;
            }
        }

        glGenVertexArrays(1, &objsquarevao);
        glBindVertexArray(objsquarevao);
        glGenBuffers(2, objsquarevbo);

        glBindBuffer(GL_ARRAY_BUFFER, objsquarevbo[0]);
        glBufferData(GL_ARRAY_BUFFER, objShape.size() * sizeof(GLfloat), objShape.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, objsquarevbo[1]);
        glBufferData(GL_ARRAY_BUFFER, objColors.size() * sizeof(GLfloat), objColors.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        free(mySquareModel.vertices);
        free(mySquareModel.faces);
    }

    // Cylinder OBJ
    Model myCylinderModel;
    read_obj_file("cylinder.obj", &myCylinderModel);
    if (myCylinderModel.face_count > 0) {
        objCylinderVertexCount = static_cast<int>(myCylinderModel.face_count * 3);
        std::vector<GLfloat> objShape(objCylinderVertexCount * 3);
        std::vector<GLfloat> objColors(objCylinderVertexCount * 3);

        for (size_t i = 0; i < myCylinderModel.face_count; i++) {
            objShape[i * 9 + 0] = myCylinderModel.vertices[myCylinderModel.faces[i].v1].x;
            objShape[i * 9 + 1] = myCylinderModel.vertices[myCylinderModel.faces[i].v1].y;
            objShape[i * 9 + 2] = myCylinderModel.vertices[myCylinderModel.faces[i].v1].z;
            objShape[i * 9 + 3] = myCylinderModel.vertices[myCylinderModel.faces[i].v2].x;
            objShape[i * 9 + 4] = myCylinderModel.vertices[myCylinderModel.faces[i].v2].y;
            objShape[i * 9 + 5] = myCylinderModel.vertices[myCylinderModel.faces[i].v2].z;
            objShape[i * 9 + 6] = myCylinderModel.vertices[myCylinderModel.faces[i].v3].x;
            objShape[i * 9 + 7] = myCylinderModel.vertices[myCylinderModel.faces[i].v3].y;
            objShape[i * 9 + 8] = myCylinderModel.vertices[myCylinderModel.faces[i].v3].z;

            for (int k = 0; k < 3; k++) {
                objColors[i * 9 + k * 3 + 0] = 0.8f;
                objColors[i * 9 + k * 3 + 1] = 0.2f;
                objColors[i * 9 + k * 3 + 2] = 0.8f;
            }
        }

        glGenVertexArrays(1, &objcylindervao);
        glBindVertexArray(objcylindervao);
        glGenBuffers(2, objcylindervbo);

        glBindBuffer(GL_ARRAY_BUFFER, objcylindervbo[0]);
        glBufferData(GL_ARRAY_BUFFER, objShape.size() * sizeof(GLfloat), objShape.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(0);

        glBindBuffer(GL_ARRAY_BUFFER, objcylindervbo[1]);
        glBufferData(GL_ARRAY_BUFFER, objColors.size() * sizeof(GLfloat), objColors.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(1);

        free(myCylinderModel.vertices);
        free(myCylinderModel.faces);
    }
}

void resetAll() {
    target1 = true;
    target2 = true;
    shapeSet = false;

    for (int i = 0; i < 2; i++) {
        objState[i].rotX = 0.0f;
        objState[i].rotY = 0.0f;
        objState[i].orbitY = 0.0f;
        objState[i].rotXDir = 0.0f;
        objState[i].rotYDir = 0.0f;
        objState[i].orbitYDir = 0.0f;
        objState[i].scaleLocal = 1.0f;
        objState[i].scaleGlobal = 1.0f;
        objState[i].pos = glm::vec3(0.0f);
    }

    animT = false; tProgress = 0.0f; tDir = 1.0f;
    animU = false; uProgress = 0.0f; uDir = 1.0f;
    animV = false; vAngle = 0.0f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    bool isShift = (mods & GLFW_MOD_SHIFT) != 0;

    switch (key) {
    case GLFW_KEY_Q:
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

        // 선택 대상 지정
    case GLFW_KEY_1:
        target1 = true; target2 = false;
        break;
    case GLFW_KEY_2:
        target1 = false; target2 = true;
        break;
    case GLFW_KEY_3:
        target1 = true; target2 = true;
        break;

        // x/X: x축 자전 토글 애니메이션
    case GLFW_KEY_X:
        for (int i = 0; i < 2; i++) {
            if ((i == 0 && target1) || (i == 1 && target2)) {
                float targetDir = isShift ? -1.0f : 1.0f;
                if (objState[i].rotXDir == targetDir) objState[i].rotXDir = 0.0f;
                else objState[i].rotXDir = targetDir;
            }
        }
        break;

        // y/Y: y축 자전 토글 애니메이션
    case GLFW_KEY_Y:
        for (int i = 0; i < 2; i++) {
            if ((i == 0 && target1) || (i == 1 && target2)) {
                float targetDir = isShift ? -1.0f : 1.0f;
                if (objState[i].rotYDir == targetDir) objState[i].rotYDir = 0.0f;
                else objState[i].rotYDir = targetDir;
            }
        }
        break;

        // r/R: y축 공전 토글 애니메이션
    case GLFW_KEY_R:
        for (int i = 0; i < 2; i++) {
            if ((i == 0 && target1) || (i == 1 && target2)) {
                float targetDir = isShift ? -1.0f : 1.0f;
                if (objState[i].orbitYDir == targetDir) objState[i].orbitYDir = 0.0f;
                else objState[i].orbitYDir = targetDir;
            }
        }
        break;

        // 스케일 조작
    case GLFW_KEY_A:
        for (int i = 0; i < 2; i++) {
            if ((i == 0 && target1) || (i == 1 && target2)) {
                objState[i].scaleLocal += isShift ? -0.1f : 0.1f;
                if (objState[i].scaleLocal < 0.1f) objState[i].scaleLocal = 0.1f;
            }
        }
        break;

    case GLFW_KEY_B:
        for (int i = 0; i < 2; i++) {
            if ((i == 0 && target1) || (i == 1 && target2)) {
                objState[i].scaleGlobal += isShift ? -0.1f : 0.1f;
                if (objState[i].scaleGlobal < 0.1f) objState[i].scaleGlobal = 0.1f;
            }
        }
        break;

        // 이동 조작
    case GLFW_KEY_D:
        for (int i = 0; i < 2; i++) {
            if ((i == 0 && target1) || (i == 1 && target2)) {
                objState[i].pos.x += isShift ? -0.1f : 0.1f;
            }
        }
        break;

    case GLFW_KEY_E:
        for (int i = 0; i < 2; i++) {
            if ((i == 0 && target1) || (i == 1 && target2)) {
                objState[i].pos.y += isShift ? -0.1f : 0.1f;
            }
        }
        break;

        // 특수 애니메이션 제어 (t: 직선 위치교차, u: 위아래 위치교차, v: 복합)
    case GLFW_KEY_T:
        if (animT) {
            animT = false; // 진행 중 누르면 일시정지
        }
        else {
            animT = true;
            animU = false; animV = false;
            // 이미 반대편 끝에 도착한 경우 반대 방향으로 되돌아감
            if (tProgress >= 1.0f) tDir = -1.0f;
            else if (tProgress <= 0.0f) tDir = 1.0f;
        }
        break;

    case GLFW_KEY_U:
        if (animU) {
            animU = false; // 진행 중 누르면 일시정지
        }
        else {
            animU = true;
            animT = false; animV = false;
            // 이미 반대편 끝에 도착한 경우 반대 방향으로 되돌아감
            if (uProgress >= 1.0f) uDir = -1.0f;
            else if (uProgress <= 0.0f) uDir = 1.0f;
        }
        break;

    case GLFW_KEY_V:
        animV = !animV;
        if (animV) { animT = false; animU = false; }
        break;

    case GLFW_KEY_C:
        shapeSet = !shapeSet;
        break;

    case GLFW_KEY_S:
        resetAll();
        break;
    }
}

void UpdateAnimations(float deltaTime) {
    float rotSpeed = 90.0f; // 초당 90도 회전

    // 1. 회전 애니메이션 업데이트 (x, y 자전 및 r 공전)
    for (int i = 0; i < 2; i++) {
        if (objState[i].rotXDir != 0.0f) {
            objState[i].rotX += objState[i].rotXDir * rotSpeed * deltaTime;
        }
        if (objState[i].rotYDir != 0.0f) {
            objState[i].rotY += objState[i].rotYDir * rotSpeed * deltaTime;
        }
        if (objState[i].orbitYDir != 0.0f) {
            objState[i].orbitY += objState[i].orbitYDir * rotSpeed * deltaTime;
        }
    }

    // 2. t 애니메이션: 이동 완료(1.0 또는 0.0) 시 정지
    if (animT) {
        tProgress += tDir * deltaTime * 0.8f;
        if (tProgress >= 1.0f) {
            tProgress = 1.0f;
            animT = false; // 이동 완료 후 멈춤
        }
        else if (tProgress <= 0.0f) {
            tProgress = 0.0f;
            animT = false; // 출발지 복귀 완료 후 멈춤
        }
    }

    // 3. u 애니메이션: 이동 완료(1.0 또는 0.0) 시 정지
    if (animU) {
        uProgress += uDir * deltaTime * 0.8f;
        if (uProgress >= 1.0f) {
            uProgress = 1.0f;
            animU = false; // 이동 완료 후 멈춤
        }
        else if (uProgress <= 0.0f) {
            uProgress = 0.0f;
            animU = false; // 출발지 복귀 완료 후 멈춤
        }
    }

    // 4. v 애니메이션
    if (animV) {
        vAngle += 90.0f * deltaTime;
        if (vAngle >= 360.0f) vAngle -= 360.0f;
    }
}

void drawScene(GLFWwindow* window, float deltaTime) {
    UpdateAnimations(deltaTime);
    glEnable(GL_DEPTH_TEST);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgramID);

    glm::mat4 view = glm::lookAt(
        glm::vec3(2.5f, 2.0f, 2.5f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    GLuint transformLoc = glGetUniformLocation(shaderProgramID, "transform");

    glm::mat4 baseTilt = glm::mat4(1.0f);
    baseTilt = glm::rotate(baseTilt, glm::radians(20.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    baseTilt = glm::rotate(baseTilt, glm::radians(25.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // 1. 축 렌더링
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glm::mat4 axisMVP = projection * view * baseTilt;
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(axisMVP));
    glBindVertexArray(axisvao);
    glLineWidth(1.5f);
    glDrawArrays(GL_LINES, 0, 6);

    // 2. 좌/우 두 객체 렌더링
    for (int i = 0; i < 2; i++) {
        float baseSide = (i == 0) ? -BASE_OFFSET : BASE_OFFSET;

        glm::mat4 model = glm::mat4(1.0f);

        // (b/B) 원점 스케일
        model = glm::scale(model, glm::vec3(objState[i].scaleGlobal));

        // Y축 공전 (r/R 누적 및 v애니메이션)
        float currentOrbit = objState[i].orbitY;
        if (animV) {
            currentOrbit += (i == 0) ? vAngle : -vAngle;
        }
        model = glm::rotate(model, glm::radians(currentOrbit), glm::vec3(0.0f, 1.0f, 0.0f));

        // 위치 계산 (기본 위치 + 사용자 이동 + t/u 애니메이션 오프셋)
        glm::vec3 currentPos = glm::vec3(baseSide, 0.0f, 0.0f) + objState[i].pos;

        if (tProgress > 0.0f) {
            float offset = -2.0f * baseSide * tProgress;
            currentPos.x += offset;
        }
        if (uProgress > 0.0f) {
            float arcY = sin(uProgress * 3.1415926f) * ((i == 0) ? 1.0f : -1.0f);
            float offsetX = -2.0f * baseSide * uProgress;
            currentPos.x += offsetX;
            currentPos.y += arcY;
        }

        model = glm::translate(model, currentPos);

        // (a/A) 제자리 로컬 스케일
        float currentLocalScale = objState[i].scaleLocal;
        if (animV) {
            float vFactor = (i == 0) ? (1.0f + 0.5f * sin(glm::radians(vAngle))) : (1.0f - 0.5f * sin(glm::radians(vAngle)));
            currentLocalScale *= vFactor;
        }
        model = glm::scale(model, glm::vec3(currentLocalScale));

        // (x/X, y/Y) 제자리 자전
        float currentRotX = objState[i].rotX;
        float currentRotY = objState[i].rotY;
        if (animV) {
            currentRotY += vAngle * 2.0f;
        }
        model = glm::rotate(model, glm::radians(currentRotX), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::rotate(model, glm::radians(currentRotY), glm::vec3(0.0f, 1.0f, 0.0f));

        // OBJ 자체 메시 스케일링
        model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));

        glm::mat4 objMVP = projection * view * baseTilt * model;
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(objMVP));

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        if (!shapeSet) {
            if (i == 0 && objConeVertexCount > 0) { // 좌: 원뿔
                glBindVertexArray(objconevao);
                glDrawArrays(GL_TRIANGLES, 0, objConeVertexCount);
            }
            else if (i == 1 && objSphereVertexCount > 0) { // 우: 구
                glBindVertexArray(objspherevao);
                glDrawArrays(GL_TRIANGLES, 0, objSphereVertexCount);
            }
        }
        else {
            if (i == 0 && objCylinderVertexCount > 0) { // 좌: 원기둥
                glBindVertexArray(objcylindervao);
                glDrawArrays(GL_TRIANGLES, 0, objCylinderVertexCount);
            }
            else if (i == 1 && objSquareVertexCount > 0) { // 우: 육면체
                glBindVertexArray(objsquarevao);
                glDrawArrays(GL_TRIANGLES, 0, objSquareVertexCount);
            }
        }
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

int main() {
    srand(static_cast<unsigned int>(time(NULL)));

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(600, 600, "Practice 17", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    InitShader();
    InitBuffer();

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