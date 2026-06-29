#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <fstream>
#include <sstream>

static std::string readFile(const char* path)
{
    std::ifstream file(path);

    std::cout << "Loading shader: " << path << "\n";

    if (!file.is_open())
    {
        std::cout << "FAILED TO OPEN: " << path << "\n";
        return "";
    }

    std::stringstream ss;
    ss << file.rdbuf();

    std::string content = ss.str();
    return content;
}

static GLuint compileShader(GLenum type, const char* src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success)
    {
        GLint len = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

        std::string log(len, ' ');
        glGetShaderInfoLog(shader, len, &len, log.data());

        std::cout << "Shader compile error:\n" << log << "\n";
    }

    return shader;
}

GLuint createShaderProgram(const char* vsPath, const char* fsPath)
{
    std::string vsCode = readFile(vsPath);
    std::string fsCode = readFile(fsPath);

    GLuint vs = compileShader(GL_VERTEX_SHADER, vsCode.c_str());
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsCode.c_str());

    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success)
    {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        std::cout << "Link error:\n" << log << "\n";
    }

    glDeleteShader(vs);
    glDeleteShader(fs);

    return program;
}