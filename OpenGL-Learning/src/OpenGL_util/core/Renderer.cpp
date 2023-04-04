#include "Renderer.h"
#include <iostream>
#include "Renderer.h"
#include "Renderer.h"

void BatchRenderer::Clear()
{
    GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

void BatchRenderer::Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader, int mode, int count)
{
    size_t c = count == -1 ? ib.GetCount() : (size_t)count;
    shader.Bind();
    va.Bind();
    ib.Bind();
    GLCall(glDrawElements(mode, c, GL_UNSIGNED_INT, nullptr));
}

void BatchRenderer::Draw(const VertexArray& va, const IndexBuffer& ib, const Shader& shader, size_t count)
{
    shader.Bind();
    va.Bind();
    ib.Bind();
    GLCall(glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr));
}

void BatchRenderer::DrawArray(const VertexArray& va, const Shader& shader, size_t first, size_t count)
{
    shader.Bind();
    va.Bind();
    glDrawArrays(GL_TRIANGLES, (int)first, (int)count);
}
