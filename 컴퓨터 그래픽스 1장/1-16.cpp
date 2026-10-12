#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <GL/glew.h>
#include <GL/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint shaderProgramID;
GLuint cubevao, cubevbo[2];
GLuint pyramidvao, pyramidvbo[2];
GLuint axisvao, axisvbo[2];

float xPos = 0.0f, yPos = 0.0f, zPos = 0.0f;
bool isDepthTest = true;
bool isPyramidMode = false;

bool rotateY = false;
float yAngle = 0.0f;

bool animT = false; float angleT = 0.0f;
bool animF = false; float angleF = 0.0f; float dirF = 1.0f;
bool animS = false; float angleS = 0.0f;
bool animB = false; float scaleB = 1.0f; float dirB = -1.0f;

bool anim1 = false; float angle1 = 0.0f; int sidePair1 = 0;
bool anim2 = false; float angle2 = 0.0f; int sidePair2 = 0;
bool anim4 = false; float angle4 = 0.0f;
bool anim5 = false; float angle5 = 0.0f;

bool anim6 = false; float angle6 = 0.0f; float dir6 = 1.0f;
bool anim7 = false; float angle7 = 0.0f; int sideIndex7 = 0; float dir7 = 1.0f;
bool anim8 = false; float currAngle8 = 0.0f; float targetAngle8 = 0.0f; int pair8 = 0;
bool anim9 = false; float angle9 = 0.0f; float dir9 = 1.0f;

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

void InitBuffer() {
    GLfloat cubeShape[36][3] = {
        {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
        { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f},
        {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f},
        { 0.5f, 0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f},
        {-0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f},
        { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f},
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
        { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f,-0.5f},
        { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f},
        { 0.5f, 0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f},
        {-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f},
        {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}
    };

    GLfloat cubeColors[36][3];
    for (int i = 0; i < 36; i++) {
        int face = i / 6;
        if (face == 0) { cubeColors[i][0] = 1.0f; cubeColors[i][1] = 0.0f; cubeColors[i][2] = 0.0f; }
        else if (face == 1) { cubeColors[i][0] = 0.0f; cubeColors[i][1] = 1.0f; cubeColors[i][2] = 0.0f; }
        else if (face == 2) { cubeColors[i][0] = 0.0f; cubeColors[i][1] = 0.0f; cubeColors[i][2] = 1.0f; }
        else if (face == 3) { cubeColors[i][0] = 1.0f; cubeColors[i][1] = 1.0f; cubeColors[i][2] = 0.0f; }
        else if (face == 4) { cubeColors[i][0] = 1.0f; cubeColors[i][1] = 0.0f; cubeColors[i][2] = 1.0f; }
        else { cubeColors[i][0] = 0.0f; cubeColors[i][1] = 1.0f; cubeColors[i][2] = 1.0f; }
    }

    GLfloat pyramid[18][3] = {
        {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f},
        { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f,-0.5f},
        { 0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, 0.5f}, { 0.0f, 0.5f, 0.0f},
        { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, 0.5f}, { 0.0f, 0.5f, 0.0f},
        {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.0f, 0.5f, 0.0f},
        {-0.5f, -0.5f, 0.5f}, {-0.5f, -0.5f, -0.5f}, { 0.0f, 0.5f, 0.0f}
    };

    GLfloat pyramidColors[18][3];
    for (int i = 0; i < 18; i++) {
        if (i < 6) {
            pyramidColors[i][0] = 0.5f; pyramidColors[i][1] = 0.5f; pyramidColors[i][2] = 0.5f;
        }
        else {
            int sideFace = (i - 6) / 3;
            if (sideFace == 0) { pyramidColors[i][0] = 1.0f; pyramidColors[i][1] = 0.0f; pyramidColors[i][2] = 0.0f; }
            else if (sideFace == 1) { pyramidColors[i][0] = 0.0f; pyramidColors[i][1] = 1.0f; pyramidColors[i][2] = 0.0f; }
            else if (sideFace == 2) { pyramidColors[i][0] = 0.0f; pyramidColors[i][1] = 0.0f; pyramidColors[i][2] = 1.0f; }
            else { pyramidColors[i][0] = 1.0f; pyramidColors[i][1] = 1.0f; pyramidColors[i][2] = 0.0f; }
        }
    }

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

    glGenVertexArrays(1, &cubevao);
    glBindVertexArray(cubevao);
    glGenBuffers(2, cubevbo);

    glBindBuffer(GL_ARRAY_BUFFER, cubevbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeShape), cubeShape, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, cubevbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeColors), cubeColors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    glGenVertexArrays(1, &pyramidvao);
    glBindVertexArray(pyramidvao);
    glGenBuffers(2, pyramidvbo);

    glBindBuffer(GL_ARRAY_BUFFER, pyramidvbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramid), pyramid, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, pyramidvbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pyramidColors), pyramidColors, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

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
}

void ResetAllAnimations() {
    rotateY = false; yAngle = 0.0f;
    animT = false; angleT = 0.0f;
    animF = false; angleF = 0.0f; dirF = 1.0f;
    animS = false; angleS = 0.0f;
    animB = false; scaleB = 1.0f; dirB = -1.0f;
    anim1 = false; angle1 = 0.0f;
    anim2 = false; angle2 = 0.0f;
    anim4 = false; angle4 = 0.0f;
    anim5 = false; angle5 = 0.0f;

    anim6 = false; angle6 = 0.0f; dir6 = 1.0f;
    anim7 = false; angle7 = 0.0f; sideIndex7 = 0; dir7 = 1.0f;
    anim8 = false; currAngle8 = 0.0f; targetAngle8 = 0.0f; pair8 = 0;
    anim9 = false; angle9 = 0.0f; dir9 = 1.0f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
    case GLFW_KEY_Q:
    case GLFW_KEY_ESCAPE:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

    case GLFW_KEY_H: isDepthTest = !isDepthTest; break;
    case GLFW_KEY_P: isPyramidMode = !isPyramidMode; break;
    case GLFW_KEY_Y: rotateY = !rotateY; break;
    case GLFW_KEY_R: ResetAllAnimations(); isPyramidMode = false; break;

    case GLFW_KEY_T:
        isPyramidMode = false;
        angle2 = 0.0f; angleF = 0.0f; scaleB = 1.0f;
        animT = !animT;
        break;

    case GLFW_KEY_F:
        isPyramidMode = false;
        angle2 = 0.0f;
        if (!animF) { animF = true; angleF = 0.0f; dirF = 1.0f; }
        break;

    case GLFW_KEY_S:
        isPyramidMode = false;
        angle2 = 0.0f; angleF = 0.0f; scaleB = 1.0f;
        animS = !animS;
        break;

    case GLFW_KEY_B:
        isPyramidMode = false;
        angle2 = 0.0f;
        if (!animB) { animB = true; scaleB = 1.0f; dirB = -1.0f; }
        break;

    case GLFW_KEY_1:
        isPyramidMode = false;
        angle2 = 0.0f; angleF = 0.0f; scaleB = 1.0f;
        anim1 = !anim1;
        if (anim1) sidePair1 = rand() % 4;
        break;

    case GLFW_KEY_2:
        isPyramidMode = false;
        angleF = 0.0f; scaleB = 1.0f;
        if (!anim2) { anim2 = true; sidePair2 = rand() % 4; angle2 = 0.0f; }
        break;

    case GLFW_KEY_4:
        isPyramidMode = false;
        angle2 = 0.0f; angleF = 0.0f; scaleB = 1.0f;
        anim4 = !anim4;
        break;

    case GLFW_KEY_5:
        isPyramidMode = false;
        angle2 = 0.0f; angleF = 0.0f; scaleB = 1.0f;
        anim5 = !anim5;
        break;

    case GLFW_KEY_6:
        isPyramidMode = true;
        anim8 = false; currAngle8 = 0.0f; targetAngle8 = 0.0f;
        anim7 = false; angle7 = 0.0f; sideIndex7 = 0; dir7 = 1.0f;
        anim9 = false; angle9 = 0.0f; dir9 = 1.0f;
        anim6 = !anim6;
        if (!anim6) { angle6 = 0.0f; dir6 = 1.0f; }
        break;

    case GLFW_KEY_7:
        isPyramidMode = true;
        anim8 = false; currAngle8 = 0.0f; targetAngle8 = 0.0f;
        anim6 = false; angle6 = 0.0f; dir6 = 1.0f;
        anim9 = false; angle9 = 0.0f; dir9 = 1.0f;
        anim7 = !anim7;
        if (!anim7) { angle7 = 0.0f; sideIndex7 = 0; dir7 = 1.0f; }
        break;

    case GLFW_KEY_8:
        isPyramidMode = true;
        anim6 = false; angle6 = 0.0f; dir6 = 1.0f;
        anim7 = false; angle7 = 0.0f; sideIndex7 = 0; dir7 = 1.0f;
        anim9 = false; angle9 = 0.0f; dir9 = 1.0f;
        if (!anim8) {
            anim8 = true;
            pair8 = rand() % 2;
            targetAngle8 += 180.0f;
        }
        break;

    case GLFW_KEY_9:
        isPyramidMode = true;
        anim8 = false; currAngle8 = 0.0f; targetAngle8 = 0.0f;
        anim6 = false; angle6 = 0.0f; dir6 = 1.0f;
        anim7 = false; angle7 = 0.0f; sideIndex7 = 0; dir7 = 1.0f;
        anim9 = !anim9;
        if (!anim9) { angle9 = 0.0f; dir9 = 1.0f; }
        break;
    }
}

void UpdateAnimations(float deltaTime) {
    float speedMultiplier = deltaTime * 60.0f;

    if (rotateY) {
        yAngle += 1.0f * speedMultiplier;
        if (yAngle >= 360.0f) yAngle -= 360.0f;
    }

    if (animT) { angleT += 1.2f * speedMultiplier; if (angleT >= 360.0f) angleT -= 360.0f; }

    if (animF) {
        angleF += dirF * 1.2f * speedMultiplier;
        if (angleF >= 120.0f) { angleF = 120.0f; dirF = -1.0f; }
        else if (angleF <= 0.0f && dirF < 0.0f) { angleF = 0.0f; animF = false; }
    }

    if (animS) { angleS += 1.2f * speedMultiplier; if (angleS >= 360.0f) angleS -= 360.0f; }

    if (animB) {
        scaleB += dirB * 0.02f * speedMultiplier;
        if (scaleB <= 0.0f) { scaleB = 0.0f; dirB = 1.0f; }
        else if (scaleB >= 1.0f && dirB > 0.0f) { scaleB = 1.0f; animB = false; }
    }

    if (anim1) { angle1 += 1.2f * speedMultiplier; if (angle1 >= 360.0f) angle1 -= 360.0f; }

    if (anim2) {
        angle2 += 1.5f * speedMultiplier;
        if (angle2 >= 90.0f) { angle2 = 90.0f; anim2 = false; }
    }

    if (anim4) { angle4 += 1.2f * speedMultiplier; if (angle4 >= 360.0f) angle4 -= 360.0f; }
    if (anim5) { angle5 += 1.2f * speedMultiplier; if (angle5 >= 360.0f) angle5 -= 360.0f; }

    if (anim6) {
        angle6 += dir6 * 1.2f * speedMultiplier;
        if (angle6 >= 180.0f) {
            angle6 = 180.0f;
            dir6 = -1.0f;
        }
        else if (angle6 <= 0.0f && dir6 < 0.0f) {
            angle6 = 0.0f;
            dir6 = 1.0f;
            anim6 = false;
        }
    }

    if (anim7) {
        angle7 += dir7 * 1.2f * speedMultiplier;
        if (angle7 >= 90.0f) {
            angle7 = 90.0f;
            dir7 = -1.0f;
        }
        else if (angle7 <= 0.0f && dir7 < 0.0f) {
            angle7 = 0.0f;
            dir7 = 1.0f;

            if (sideIndex7 == 3) {
                sideIndex7 = 0;
                anim7 = false;
            }
            else {
                sideIndex7++;
            }
        }
    }

    if (anim8) {
        currAngle8 += 1.3f * speedMultiplier;
        if (currAngle8 >= targetAngle8) {
            currAngle8 = targetAngle8;
            anim8 = false;
        }
    }

    if (anim9) {
        angle9 += dir9 * 1.1f * speedMultiplier;
        if (angle9 >= 60.0f) {
            angle9 = 60.0f;
            dir9 = -1.0f;
        }
        else if (angle9 <= 0.0f && dir9 < 0.0f) {
            angle9 = 0.0f;
            dir9 = 1.0f;
            anim9 = false;
        }
    }
}

void drawScene(GLFWwindow* window, float deltaTime) {
    UpdateAnimations(deltaTime);

    if (isDepthTest) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);

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

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glm::mat4 axisMVP = projection * view * glm::mat4(1.0f);
    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(axisMVP));
    glBindVertexArray(axisvao);
    glLineWidth(1.5f);
    glDrawArrays(GL_LINES, 0, 6);

    glm::mat4 baseModel = glm::mat4(1.0f);
    baseModel = glm::translate(baseModel, glm::vec3(xPos, yPos, zPos));
    baseModel = glm::rotate(baseModel, glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    baseModel = glm::rotate(baseModel, glm::radians(yAngle), glm::vec3(0.0f, 1.0f, 0.0f));

    if (!isPyramidMode) {
        glBindVertexArray(cubevao);
        for (int i = 0; i < 6; i++) {
            glm::mat4 faceModel = baseModel;

            if (anim5 && (i == 0 || i == 1 || i == 2 || i == 3)) {
                faceModel = glm::rotate(faceModel, glm::radians(angle5), glm::vec3(1.0f, 0.0f, 0.0f));
            }
            if (anim4 && (i == 0 || i == 1 || i == 4 || i == 5)) {
                faceModel = glm::rotate(faceModel, glm::radians(angle4), glm::vec3(0.0f, 1.0f, 0.0f));
            }

            if (anim1) {
                int f1 = sidePair1, f2 = (sidePair1 + 1) % 4;
                int sideMap[4] = { 0, 4, 1, 5 };
                if (i == sideMap[f1] || i == sideMap[f2]) {
                    faceModel = glm::rotate(faceModel, glm::radians(angle1), glm::vec3(0.0f, 1.0f, 0.0f));
                }
            }

            if (anim2 || angle2 > 0.0f) {
                int sideMap[4] = { 0, 4, 1, 5 };
                int f1 = sideMap[sidePair2];
                int f2 = sideMap[(sidePair2 + 1) % 4];
                if (i == f1) faceModel = glm::rotate(faceModel, glm::radians(angle2), glm::vec3(0.0f, 1.0f, 0.0f));
                else if (i == f2) faceModel = glm::rotate(faceModel, glm::radians(-angle2), glm::vec3(0.0f, 1.0f, 0.0f));
            }
            if (i == 2 && animT) {
                faceModel = glm::translate(faceModel, glm::vec3(0.0f, 0.5f, 0.0f));
                faceModel = glm::rotate(faceModel, glm::radians(angleT), glm::vec3(0.0f, 0.0f, 1.0f));
                faceModel = glm::translate(faceModel, glm::vec3(0.0f, -0.5f, 0.0f));
            }
            else if (i == 0 && (animF || angleF > 0.0f)) {
                faceModel = glm::translate(faceModel, glm::vec3(0.0f, 0.5f, 0.5f));
                faceModel = glm::rotate(faceModel, glm::radians(-angleF), glm::vec3(1.0f, 0.0f, 0.0f));
                faceModel = glm::translate(faceModel, glm::vec3(0.0f, -0.5f, -0.5f));
            }
            else if (i == 4 && animS) {
                faceModel = glm::translate(faceModel, glm::vec3(0.5f, 0.0f, 0.0f));
                faceModel = glm::rotate(faceModel, glm::radians(angleS), glm::vec3(1.0f, 0.0f, 0.0f));
                faceModel = glm::translate(faceModel, glm::vec3(-0.5f, 0.0f, 0.0f));
            }
            else if (i == 1 && (animB || scaleB < 1.0f)) {
                faceModel = glm::translate(faceModel, glm::vec3(0.0f, 0.0f, -0.5f));
                faceModel = glm::scale(faceModel, glm::vec3(scaleB, scaleB, 1.0f));
                faceModel = glm::translate(faceModel, glm::vec3(0.0f, 0.0f, 0.5f));
            }

            glm::mat4 mvp = projection * view * faceModel;
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(mvp));
            glDrawArrays(GL_TRIANGLES, i * 6, 6);
        }
    }
    else {
        glBindVertexArray(pyramidvao);

        glm::mat4 baseMVP = projection * view * baseModel;
        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(baseMVP));
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glm::vec3 edgePos[4] = {
            glm::vec3(0.0f, -0.5f, 0.5f),
            glm::vec3(0.5f, -0.5f, 0.0f),
            glm::vec3(0.0f, -0.5f, -0.5f),
            glm::vec3(-0.5f, -0.5f, 0.0f)
        };

        glm::vec3 rotAxis[4] = {
            glm::vec3(1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, -1.0f),
            glm::vec3(-1.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        };

        for (int i = 0; i < 4; i++) {
            glm::mat4 sideModel = baseModel;

            if (anim6) {
                sideModel = glm::translate(sideModel, edgePos[i]);
                sideModel = glm::rotate(sideModel, glm::radians(angle6), rotAxis[i]);
                sideModel = glm::translate(sideModel, -edgePos[i]);
            }

            if (anim7 && (i == sideIndex7)) {
                sideModel = glm::translate(sideModel, edgePos[i]);
                sideModel = glm::rotate(sideModel, glm::radians(angle7), rotAxis[i]);
                sideModel = glm::translate(sideModel, -edgePos[i]);
            }

            bool isTargetPair = false;
            if (pair8 == 0 && (i == 0 || i == 2)) isTargetPair = true;
            if (pair8 == 1 && (i == 1 || i == 3)) isTargetPair = true;

            if (isTargetPair) {
                glm::vec3 apex(0.0f, 0.5f, 0.0f);
                sideModel = glm::translate(sideModel, apex);
                sideModel = glm::rotate(sideModel, glm::radians(currAngle8), glm::vec3(0.0f, 1.0f, 0.0f));
                sideModel = glm::translate(sideModel, -apex);
            }

            if (anim9) {
                glm::vec3 apex(0.0f, 0.5f, 0.0f);
                sideModel = glm::translate(sideModel, apex);
                sideModel = glm::rotate(sideModel, glm::radians(-angle9), rotAxis[i]);
                sideModel = glm::translate(sideModel, -apex);
            }

            glm::mat4 sideMVP = projection * view * sideModel;
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(sideMVP));
            glDrawArrays(GL_TRIANGLES, 6 + i * 3, 3);
        }
    }
}

int main() {
    srand(static_cast<unsigned int>(time(NULL)));

    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(600, 600, "Practice 16", nullptr, nullptr);
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