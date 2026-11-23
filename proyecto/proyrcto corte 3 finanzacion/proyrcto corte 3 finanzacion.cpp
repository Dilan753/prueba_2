#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cmath>

// tinygltf: estas 3 lineas van en UN solo .cpp del proyecto, antes del include.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE

#include "shaders.h"
#include "modelo.h"
#include "camara.h"
#include "bezier.h"
#include "luz.h"
#include "animacion.h"

int main(int argc, char** argv) {
    // Ruta por defecto, relativa a la carpeta del proyecto
    std::string ruta = (argc > 1)
        ? argv[1]
        : "gears_of_war_custom_lancer/scene.gltf";

    // --- ventana y contexto OpenGL ---
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* ventana = glfwCreateWindow(1024, 768, "Esena", nullptr, nullptr);
    if (!ventana) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(ventana);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glEnable(GL_DEPTH_TEST);

    // --- Dear ImGui (despues de crear el contexto OpenGL) ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(ventana, true);   // true = ImGui instala sus callbacks de teclado/mouse
    ImGui_ImplOpenGL3_Init("#version 330");

    GLuint shader = crearPrograma();

    //   23 = "Custom Lancer_14" (el arma)       30, 34, 38, 50 = sus accesorios
    const std::vector<int> NODOS_PROTAGONISTA = { 23, 30, 34, 38, 50 };

    Modelo modelo;
    if (!modelo.cargar(ruta, NODOS_PROTAGONISTA)) return -1;
    glm::mat4 normalizar = modelo.normalizar();
    glm::mat4 giro = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0, 1, 0));
    glm::mat4 mundoBase = giro * normalizar;   // de coordenadas del modelo a coordenadas del mundo
    glm::vec3 centroProta = modelo.centroProta(mundoBase);

    // Curva de Bezier 
    GLuint shaderLineas = crearProgramaLineas();
    //  Ajusta las coordenadas a mano.
    CurvaBezier curva(glm::vec3(0.0f, -0.8f, 0.0f),    // P0: punto mas bajo 
        glm::vec3(-0.6f, -1.0f, 0.0f),   // P1
        glm::vec3(-0.6f, 1.0f, 0.0f),    // P2
        glm::vec3(0.0f, 1.0f, 0.0f));    // P3: punto mas alto 

    const glm::vec3 ejeGiro = glm::normalize(glm::vec3(-1.0f, 0.0f, -1.0f));

    Luz luz;
    Animacion animacion;
    Camara camara;

    
    bool  expositor = false;      // true = el modelo completo gira
    float velExpositor = 45.0f;   // grados por segundo
    float anguloExpositor = 0.0f; // giro acumulado en grados

    float tiempoAnterior = (float)glfwGetTime();

    // --- bucle principal ---
    while (!glfwWindowShouldClose(ventana)) {
        float tiempoActual = (float)glfwGetTime();
        float dt = tiempoActual - tiempoAnterior;
        tiempoAnterior = tiempoActual;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Controles");

        // Animacion 
        if (!animacion.activa) {
            if (ImGui::Button("Iniciar animacion")) {
                expositor = false;          // no se mezcla con el expositor
                anguloExpositor = 0.0f;
                animacion.iniciar();
            }
        }
        else {
            if (ImGui::Button("Detener animacion")) animacion.detener();
        }

        // Expositor: todo el modelo gira
        ImGui::BeginDisabled(animacion.activa);
        ImGui::Checkbox("Girar modelo", &expositor);
        ImGui::SliderFloat("Velocidad", &velExpositor, -180.0f, 180.0f);
        ImGui::EndDisabled();

        // Luz
        ImGui::Text("Luz");
        ImGui::SliderFloat3("Posicion", glm::value_ptr(luz.posicion), -6.0f, 6.0f);
        ImGui::ColorEdit3("Color", glm::value_ptr(luz.color));
        ImGui::SliderFloat("Intensidad", &luz.intensidad, 0.0f, 3.0f);
        if (ImGui::Button("Reiniciar luz")) {
            luz.posicion = glm::vec3(2.0f, 2.0f, 2.0f);
            luz.color = glm::vec3(1.0f);
            luz.intensidad = 1.0f;
        }

        ImGui::End();
        ImGui::Render();

        // =====================================================================
        //  ENTRADA Y ACTUALIZACION
        // =====================================================================
        if (glfwGetKey(ventana, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(ventana, true);

        // WASD solo fuera de la animacion y si ImGui no esta usando el teclado 
        if (!animacion.activa && !io.WantCaptureKeyboard)
            camara.procesarTeclado(ventana, dt);

        animacion.actualizar(dt);

        if (expositor && !animacion.activa)
            anguloExpositor = std::fmod(anguloExpositor + velExpositor * dt, 360.0f);

        // Matriz del modelo completo: el expositor lo gira sobre su eje vertical.
        // El modelo ya esta centrado en el origen, asi que gira sobre si mismo.
        glm::mat4 mundo = glm::rotate(glm::mat4(1.0f), glm::radians(anguloExpositor), glm::vec3(0, 1, 0))
            * mundoBase;

        // Donde esta el arma y cuanto ha girado segun el progreso t (de 0 a 1)
        glm::vec3 posProta = curva.punto(1.0f - animacion.t);   // sobre la curva, de arriba a abajo
        float angulo = glm::radians(45.0f - 90.0f * animacion.t);   

        // =====================================================================
        //  DIBUJO
        // =====================================================================
        int w, h;
        glfwGetFramebufferSize(ventana, &w, &h);
        glViewport(0, 0, w, h);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // camara y proyeccion
        glm::mat4 view = animacion.activa ? camara.matrizVistaSeguir(posProta)
            : camara.matrizVista();
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)w / (float)(h > 0 ? h : 1), 0.1f, 100.0f);
        glUseProgram(shader);
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform1i(glGetUniformLocation(shader, "tex"), 0);   // textura en la unidad 0
        luz.enviar(shader);
        glm::vec3 posCamara = animacion.activa ? camara.posicionSeguir(posProta) : camara.posicion;
        glUniform3fv(glGetUniformLocation(shader, "viewPos"), 1, glm::value_ptr(posCamara));

        // Durante la animacion el arma hace tres cosas
        //   1) se centra en el origen   2) gira sobre su eje   3) se coloca sobre la curva
        // El resto del modelo se queda quieto. (El expositor esta en 0 mientras corre la animacion.)
        glm::mat4 mundoProta = animacion.activa
            ? glm::translate(glm::mat4(1.0f), posProta)
            * glm::rotate(glm::mat4(1.0f), angulo, ejeGiro)
            * glm::translate(glm::mat4(1.0f), -centroProta)
            * mundo
            : mundo;

        modelo.dibujar(shader, mundo, mundoProta);

        curva.dibujar(shaderLineas, view, proj);
        luz.dibujar(shaderLineas, view, proj);

        // La interfaz se dibuja al final, encima de la escena
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(ventana);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}