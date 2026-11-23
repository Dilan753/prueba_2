// =============================================================================
//    W / S : adelante / atras
//    A / D : izquierda / derecha
//    E / Q : subir / bajar
//    Shift : moverse mas rapido
// =============================================================================
#pragma once

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Camara {
    glm::vec3 posicion{ 0.0f, 0.6f, 3.0f };
    glm::vec3 frente{ 0.0f, -0.2f, -1.0f };   // hacia donde mira
    glm::vec3 arriba{ 0.0f, 1.0f, 0.0f };
    float velocidad = 2.5f;

    // Tercera persona: distancia fija entre la camara y el arma (arriba y hacia el frente)
    glm::vec3 distanciaSeguir{ 0.0f, 0.6f, 3.0f };

    Camara() { frente = glm::normalize(frente); }

    // Matriz de vista = rotacion * traslacion
    //   traslacion: mueve todo el mundo -pos (mover la camara a la derecha = mover el mundo a la izquierda)
    //   rotacion  : gira el mundo para que quede alineado con hacia donde mira la camara
    glm::mat4 matrizDesde(const glm::vec3& pos, const glm::vec3& direccion) const {
        glm::mat4 rotacion = glm::lookAt(glm::vec3(0.0f), direccion, arriba);
        glm::mat4 traslacion = glm::translate(glm::mat4(1.0f), -pos);
        return rotacion * traslacion;
    }

    // Matriz de vista para enviar al shader (camara libre)
    glm::mat4 matrizVista() const {
        return matrizDesde(posicion, frente);
    }

    // Posicion de la camara en tercera persona
    glm::vec3 posicionSeguir(const glm::vec3& objetivo) const {
        return objetivo + distanciaSeguir;
    }

    // Matriz de vista en tercera persona: la camara va junto al objetivo y lo mira.
    glm::mat4 matrizVistaSeguir(const glm::vec3& objetivo) const {
        glm::vec3 pos = posicionSeguir(objetivo);
        return matrizDesde(pos, objetivo - pos);   
    }

    // Revisa el teclado y mueve la camara. dt = segundos desde el frame anterior
    void procesarTeclado(GLFWwindow* ventana, float dt) {
        float v = velocidad * dt;
        
        glm::vec3 derecha = glm::normalize(glm::cross(frente, arriba));

        if (glfwGetKey(ventana, GLFW_KEY_W) == GLFW_PRESS) posicion += frente * v;
        if (glfwGetKey(ventana, GLFW_KEY_S) == GLFW_PRESS) posicion -= frente * v;
        if (glfwGetKey(ventana, GLFW_KEY_D) == GLFW_PRESS) posicion += derecha * v;
        if (glfwGetKey(ventana, GLFW_KEY_A) == GLFW_PRESS) posicion -= derecha * v;
        if (glfwGetKey(ventana, GLFW_KEY_E) == GLFW_PRESS) posicion += arriba * v;
        if (glfwGetKey(ventana, GLFW_KEY_Q) == GLFW_PRESS) posicion -= arriba * v;
    }
};
