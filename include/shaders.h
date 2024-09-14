#pragma once

#include <GL/glew.h>
#include <GL/gl.h>

#include <cstdio>
#include <string>

static constexpr auto basic_vertex_src = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;

uniform sampler2D image;
uniform vec2 screen_size;
uniform vec2 image_size;

out vec2 UV;

void main() {
    UV = vec2(aUV.x, aUV.y);
    //vec2 pos = aPos / screen_size;
    vec2 pos = aPos;
    pos = (vec2(pos.x, 1.0 - pos.y) - vec2(0.5)) * 2.0;
    gl_Position = vec4(pos, 0.0, 1.0);
}
)";

static constexpr auto image_fragment_src = R"(
#version 330 core

in vec2 UV;
out vec4 Color;

uniform sampler2D image;
uniform vec2 screen_size;
uniform vec2 image_size;

uniform int show_outline;
uniform vec2 clip_pos;
uniform vec2 clip_size;

void main() {
    vec2 uv = UV;
    float screen_aspect = screen_size.x / screen_size.y;
    float image_aspect = image_size.x / image_size.y;

    if (screen_aspect > image_aspect) {
        // Screen is wider than image
        float xmax = screen_aspect / image_aspect;
        uv.x *= xmax;

        // Black bars
        uv.x -= (xmax - 1.0) / 2.0;
    } else {
        // Screen is taller than image
        float ymax = image_aspect / screen_aspect;
        uv.y *= ymax;

        // Black bars
        uv.y -= (ymax - 1.0) / 2.0;
    }

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        Color = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        vec4 color = vec4(texture2D(image, uv).rgb, 1.0);

        if (show_outline == 1)
        {
            float dx = 0.00125;
            float dy = dx * screen_aspect;

            vec2 clip_tl = clip_pos;
            vec2 clip_br = clip_pos + clip_size;

            if (uv.x >= clip_tl.x && uv.y >= clip_tl.y && uv.x <= clip_br.x && uv.y <= clip_br.y)
            {
                if (abs(uv.x - clip_tl.x) < dx || abs(uv.y - clip_tl.y) < dy || abs(uv.x - clip_br.x) < dx || abs(uv.y - clip_br.y) < dy)
                {
                    color.rgb = vec3(0.6);
                }
            }
        }

        Color = color;
    }
}

)";

static GLuint compile_shader(const char* source, GLenum shaderType)
{
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        puts(infoLog);
    }

    return shader;
}

static int create_shader(const std::string &vertex_src, const std::string &fragment_src)
{
    int vertex_id = compile_shader(vertex_src.c_str(), GL_VERTEX_SHADER);
    int fragment_id = compile_shader(fragment_src.c_str(), GL_FRAGMENT_SHADER);

    int program = glCreateProgram();
    glAttachShader(program, vertex_id);
    glAttachShader(program, fragment_id);
    glLinkProgram(program);

    return program;
}

