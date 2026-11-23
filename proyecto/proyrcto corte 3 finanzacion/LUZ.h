// =============================================================================
//    J / L : izquierda / derecha   (eje X)
//    I / K : lejos / cerca         (eje Z)
//    U / O : bajar / subir         (eje Y)
//
//  La luz se dibuja como un punto naranja para saber donde esta
// =============================================================================
#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Luz {
    glm::vec3 posicion{ 2.0f, 2.0f, 2.0f }; 
    glm::vec3 color{ 1.0f, 1.0f, 1.0f };
    float velocidad = 3.0f;                    
    float intensidad = 1.0f;                   

    Luz() {
        // un solo vertice en el origen: el marcador de la luz
        glm::vec3 origen(0.0f);
        GLuint vbo;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);
        glGenBuffers(1, &vbo);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), glm::value_ptr(origen), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glBindVertexArray(0);
    }

    // Mueve la luz con el teclado. dt = segundos desde el frame anterior
    void procesarTeclado(GLFWwindow* ventana, float dt) {
        float v = velocidad * dt;
        if (glfwGetKey(ventana, GLFW_KEY_L) == GLFW_PRESS) posicion.x += v;
        if (glfwGetKey(ventana, GLFW_KEY_J) == GLFW_PRESS) posicion.x -= v;
        if (glfwGetKey(ventana, GLFW_KEY_O) == GLFW_PRESS) posicion.y += v;
        if (glfwGetKey(ventana, GLFW_KEY_U) == GLFW_PRESS) posicion.y -= v;
        if (glfwGetKey(ventana, GLFW_KEY_K) == GLFW_PRESS) posicion.z += v;
        if (glfwGetKey(ventana, GLFW_KEY_I) == GLFW_PRESS) posicion.z -= v;
    }

    // Envia la luz al shader del modelo (el programa ya debe estar activo)
    void enviar(GLuint shader) const {
        glUniform3fv(glGetUniformLocation(shader, "lightPos"), 1, glm::value_ptr(posicion));
        glm::vec3 c = color * intensidad;
        glUniform3fv(glGetUniformLocation(shader, "lightColor"), 1, glm::value_ptr(c));
    }

    // Dibuja el marcador (shader = programa de lineas/puntos)
    void dibujar(GLuint shaderLineas, const glm::mat4& view, const glm::mat4& proj) const {
        glUseProgram(shaderLineas);
        glm::mat4 mvp = proj * view * glm::translate(glm::mat4(1.0f), posicion);
        glUniformMatrix4fv(glGetUniformLocation(shaderLineas, "mvp"), 1, GL_FALSE, glm::value_ptr(mvp));
        glUniform3f(glGetUniformLocation(shaderLineas, "color"), 1.0f, 0.5f, 0.0f);
        glPointSize(14.0f);
        glBindVertexArray(vao);
        glDrawArrays(GL_POINTS, 0, 1);
        glBindVertexArray(0);
    }

private:
    GLuint vao = 0;
};