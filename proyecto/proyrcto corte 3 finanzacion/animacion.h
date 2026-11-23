// =============================================================================
//  t va de 0 a 1 a lo largo de la curva.
// =============================================================================
#pragma once

#include <GLFW/glfw3.h>

struct Animacion {
    bool  activa = false;
    float t = 0.0f;          // progreso en la curva: 0 = inicio, 1 = final
    float duracion = 5.0f;   // segundos para recorrer toda la curva

    void iniciar() {
        activa = true;
        t = 0.0f;
    }

    //devuelve el arma a su sitio
    void detener() {
        activa = false;
        t = 0.0f;
    }

    // Revisa si el usuario pidio iniciar
    void procesarTeclado(GLFWwindow* ventana) {
        if (!activa && glfwGetKey(ventana, GLFW_KEY_SPACE) == GLFW_PRESS)
            iniciar();
    }

    // Avanza t segun el tiempo. dt = segundos desde el frame anterior
    void actualizar(float dt) {
        if (!activa) return;
        t += dt / duracion;
        if (t >= 1.0f) {
            t = 1.0f;
            activa = false; 
        }
    }
};