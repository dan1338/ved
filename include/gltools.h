#pragma once

#include "fmt/base.h"
#include <memory>
#include <vector>

#include <GL/glew.h>
#include <GL/glext.h>

struct GLObject
{
    GLuint vao, vbo, ebo;
    size_t num_verts;
    size_t num_indices;
    GLuint draw_mode;

    void draw() const
    {
        glBindVertexArray(vao);
        glDrawElements(draw_mode, num_indices, GL_UNSIGNED_INT, nullptr);
    }
};

struct GLTexture
{
    GLuint id;

    GLTexture()
    {
        glGenTextures(1, &id);
    }

    void update(size_t w, size_t h, unsigned char *data)
    {
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
};

namespace gltools
{

static std::unique_ptr<GLObject> make_rect(float x, float y, float w, float h)
{
    float verts[] = {
        x, y + h,     0.0, 0.0,
        x + w, y + h, 1.0, 0.0,
        x + w, y,     1.0, 1.0,
        x, y,         0.0, 1.0,
    };

    int indices[] = {
        0, 1, 2, 2, 3, 0
    };

    auto obj = std::make_unique<GLObject>();
    obj->draw_mode = GL_TRIANGLES;
    obj->num_verts = 4;
    obj->num_indices = 6;

    glGenVertexArrays(1, &obj->vao);
    glBindVertexArray(obj->vao);

    glGenBuffers(1, &obj->vbo);
    glBindBuffer(GL_ARRAY_BUFFER, obj->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof verts, verts, GL_STATIC_DRAW);

    glGenBuffers(1, &obj->ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, obj->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof indices, indices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, (void*)(sizeof(float) * 2));

    return obj;
}

}
