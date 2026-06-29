#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>

#include "common.h"

GLuint createShaderProgram(const char* vsPath, const char* fsPath);

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        float ro[3] = { gData.camPos[0], gData.camPos[1], gData.camPos[2] };
        float rd[3] = { 
            (float)(cos(gData.camRot[0]) * sin(gData.camRot[1])), 
            (float)(sin(gData.camRot[0])), 
            (float)(cos(gData.camRot[0]) * cos(gData.camRot[1])) 
        };

        float result[3];
        raycast(ro, rd, gData.portals, result, gData.dimension);

        if (result[0] > 0.5f) {
            float hitPoint[3] = {
                ro[0] + rd[0] * result[1],
                ro[1] + rd[1] * result[1],
                ro[2] + rd[2] * result[1]
            };

            Portal newPortal(hitPoint[0], hitPoint[1], hitPoint[2], rd[0]);
            gData.portals.push_back(newPortal);
        }
    }
}

void update(GLFWwindow* window, float deltaTime, int num_portals)
{
    float pitch = gData.camRot[0];
    float yaw = gData.camRot[1];

    float last[3] = {gData.camPos[0], gData.camPos[1], gData.camPos[2]};

    float moveSpeed = 0.1f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        gData.velocity[0] += sin(yaw) * moveSpeed * deltaTime;
        gData.velocity[2] += cos(yaw) * moveSpeed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        gData.velocity[0] -= sin(yaw) * moveSpeed * deltaTime;
        gData.velocity[2] -= cos(yaw) * moveSpeed * deltaTime;
    }

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        gData.velocity[0] -= cos(yaw) * moveSpeed * deltaTime;
        gData.velocity[2] += sin(yaw) * moveSpeed * deltaTime;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        gData.velocity[0] += cos(yaw) * moveSpeed * deltaTime;
        gData.velocity[2] -= sin(yaw) * moveSpeed * deltaTime;
    }

    static bool sl = false;
    bool s = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;   
    if (s && !sl) {
        gData.velocity[1] += 10.0f * deltaTime;
    }

    for(int i = 0; i < num_portals; i++) {
        Portal portal = gData.portals[i];
        if(sign(gData.camPos[2] - portal.data[2]) != sign(gData.camPos[2] - portal.data[2] + gData.velocity[2]) && abs(gData.camPos[0] - portal.data[0]) < 0.5 && abs(gData.camPos[1] - portal.data[1]) < 1.5) gData.dimension = 1 - gData.dimension;
    }

    gData.velocity[1] -= 0.5f * deltaTime;
    gData.camPos[0] += gData.velocity[0]; gData.camPos[1] += gData.velocity[1]; gData.camPos[2] += gData.velocity[2];
    if(gData.camPos[1] < 1.0f) {
        gData.camPos[1] = 1.0f;
        gData.velocity[1] = 0.0f;
    }

    float f = 0.95f;
    gData.velocity[0] *= f; gData.velocity[1] *= f; gData.velocity[2] *= f;

    double mouseX, mouseY;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    int w, h;
    glfwGetWindowSize(window, &w, &h);

    float sensitivity = 0.2f;
    float offsetX = (float)(mouseX - w / 2.0) * sensitivity;
    float offsetY = (float)(mouseY - h / 2.0) * sensitivity;

    glfwSetCursorPos(window, w / 2.0, h / 2.0);

    gData.camRot[1] += offsetX * deltaTime;
    gData.camRot[0] -= offsetY * deltaTime;
    gData.camRot[0] = std::clamp(gData.camRot[0], -1.57f, 1.57f);

    sl = s;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);

    GLFWwindow* window = glfwCreateWindow(
        mode->width,
        mode->height,
        "Engine",
        monitor,
        nullptr
    );
    glfwMakeContextCurrent(window);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    gladLoadGL();

    glfwSetMouseButtonCallback(window, mouse_button_callback);

    GLuint program = createShaderProgram("shaders/render.vert", "shaders/render.frag");

    GLuint ssbo;
    glGenBuffers(1, &ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, 10000 * sizeof(Portal), nullptr, GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    gData.portals.emplace_back(0.0f, 0.0f, 0.0f, 0.0f);

    GLint uCameraPos  = glGetUniformLocation(program, "uCameraPos");
    GLint uCameraRot  = glGetUniformLocation(program, "uCameraRot");
    GLint uResolution = glGetUniformLocation(program, "uResolution");
    GLint uTime       = glGetUniformLocation(program, "uTime");
    GLint uPortalCount = glGetUniformLocation(program, "uPortalCount");
    GLint uDimension = glGetUniformLocation(program, "uDimension");

    auto start = std::chrono::high_resolution_clock::now();
    auto lastTime = start;

    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        int w, h;
        glfwGetFramebufferSize(window, &w, &h);
        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        update(window, deltaTime, (int)gData.portals.size());

        glUseProgram(program);
        glUniform1f(uTime, std::chrono::duration<float>(now - start).count());
        glUniform3f(uCameraPos, gData.camPos[0], gData.camPos[1], gData.camPos[2]);
        glUniform3f(uCameraRot, gData.camRot[0], gData.camRot[1], gData.camRot[2]);
        glUniform2f(uResolution, (float)w, (float)h);
        glUniform1i(uDimension, gData.dimension);

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, gData.portals.size() * sizeof(Portal), gData.portals.data());
        glUniform1i(uPortalCount, (int)gData.portals.size());

        glDrawArrays(GL_TRIANGLES, 0, 3);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}