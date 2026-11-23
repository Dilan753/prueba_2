// =============================================================================
//  bezier.h - curva de Bezier cubica (4 puntos de control)
//
//    P(t) = (1-t)^3 P0 + 3(1-t)^2 t P1 + 3(1-t) t^2 P2 + t^3 P3     t en [0,1]
// =============================================================================
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <vector>

class CurvaBezier {
public:
    std::array<glm::vec3, 4> p;   // puntos de control

    CurvaBezier(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3,
        int muestras = 100)
        : p{ p0, p1, p2, p3 }, numMuestras(muestras) {

        // 1) muestrear la curva: t = 0, 1/n, 2/n ... 1
        std::vector<glm::vec3> curva;
        for (int i = 0; i <= numMuestras; i++)
            curva.push_back(punto((float)i / numMuestras));

        // 2) subir a la GPU la curva y los puntos de control
        vaoCurva = crearVAO(curva);
        vaoControl = crearVAO(std::vector<glm::vec3>(p.begin(), p.end()));
    }

    // Punto de la curva para un t entre 0 y 1 (util luego para mover objetos)
    glm::vec3 punto(float t) const {
        float u = 1.0f - t;
        return u * u * u * p[0]
            + 3.0f * u * u * t * p[1]
            + 3.0f * u * t * t * p[2]
            + t * t * t * p[3];
    }

    // Dibuja todo (shader = programa de lineas)
    void dibujar(GLuint shader, const glm::mat4& view, const glm::mat4& proj) const {
        glUseProgram(shader);
        glm::mat4 mvp = proj * view;
        glUniformMatrix4fv(glGetUniformLocation(shader, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
        GLint locColor = glGetUniformLocation(shader, "color");

        // poligono de control (gris)
        glUniform3f(locColor, 0.4f, 0.4f, 0.4f);
        glBindVertexArray(vaoControl);
        glDrawArrays(GL_LINE_STRIP, 0, 4);

        // la curva (roja)
        glUniform3f(locColor, 0.85f, 0.1f, 0.1f);
        glBindVertexArray(vaoCurva);
        glDrawArrays(GL_LINE_STRIP, 0, numMuestras + 1);

        // puntos de control (azul)
        glUniform3f(locColor, 0.1f, 0.2f, 0.9f);
        glPointSize(10.0f);
        glBindVertexArray(vaoControl);
        glDrawArrays(GL_POINTS, 0, 4);

        glBindVertexArray(0);
    }

private:
    int numMuestras;
    GLuint vaoCurva = 0, vaoControl = 0;

    // Sube una lista de posiciones a la GPU (solo location 0 = posicion)
    static GLuint crearVAO(const std::vector<glm::vec3>& v) {
        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(glm::vec3), v.data(), GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

        glBindVertexArray(0);
        return vao;
    }
};