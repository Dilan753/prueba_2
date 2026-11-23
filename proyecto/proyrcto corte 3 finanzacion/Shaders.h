#pragma once

#include <glad/glad.h>
#include <iostream>

inline const char* VERTEX_SHADER = R"(#version 330 core
layout (location = 0) in vec3 aPos;      // posicion del vertice
layout (location = 1) in vec3 aNormal;   // normal (para la luz)
layout (location = 2) in vec2 aUV;       // coordenadas de textura

uniform mat4 model;   // posicion/rotacion/escala del objeto
uniform mat4 view;    
uniform mat4 proj;    

out vec3 vNormal;
out vec3 vPos;        // para la luz
out vec2 vUV;

void main() {
    vNormal = mat3(transpose(inverse(model))) * aNormal;
    vPos = vec3(model * vec4(aPos, 1.0));
    vUV = aUV;
    gl_Position = proj * view * model * vec4(aPos, 1.0);
}
)";

inline const char* FRAGMENT_SHADER = R"(#version 330 core
in vec3 vNormal;
in vec3 vPos;
in vec2 vUV;

uniform vec3 lightPos;     
uniform vec3 lightColor;   
uniform vec3 viewPos;      
uniform sampler2D tex;   //  
uniform bool hasTex;       
uniform vec4 baseColor;    

out vec4 FragColor;

void main() {
    vec4 color = baseColor;
    if (hasTex) color *= texture(tex, vUV);

    // iluminacion Phong con luz puntual: ambiente + difusa + especular
    vec3 N = normalize(vNormal);
    vec3 L = normalize(lightPos - vPos);
    float diff = max(dot(N, L), 0.0);

    float d = length(lightPos - vPos);
    float atenuacion = 1.0 / (1.0 + 0.09 * d + 0.032 * d * d);

    // ambiente: luz base que llega a todo
    vec3 ambiente = vec3(0.25);

    // difusa: mas clara cuanto mas de frente le da la luz a la superficie
    vec3 difusa = diff * atenuacion * lightColor * 1.5;

    // especular: el brillo depende de hacia donde rebota la luz respecto a la camara
    vec3 V = normalize(viewPos - vPos);   // del fragmento hacia la camara
    vec3 R = reflect(-L, N);              // direccion en que rebota la luz
    float spec = (diff > 0.0) ? pow(max(dot(V, R), 0.0), 32.0) : 0.0;   // 32 = que tan concentrado es el brillo
    vec3 especular = spec * atenuacion * lightColor * 0.5;

    FragColor = vec4(color.rgb * (ambiente + difusa) + especular, color.a);
}
)";

// -----------------------------------------------------------------------------
//  Compilar un shader (vertex o fragment)
// -----------------------------------------------------------------------------
inline GLuint compilarShader(GLenum tipo, const char* src) {
    GLuint s = glCreateShader(tipo);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);

    GLint ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, 1024, nullptr, log);
        std::cerr << "Error de shader:\n" << log << "\n";
    }
    return s;
}

// -----------------------------------------------------------------------------
//  Crear el programa completo (vertex + fragment enlazados)
// -----------------------------------------------------------------------------
inline GLuint crearProgramaDesde(const char* srcVS, const char* srcFS) {
    GLuint vs = compilarShader(GL_VERTEX_SHADER, srcVS);
    GLuint fs = compilarShader(GL_FRAGMENT_SHADER, srcFS);

    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);

    glDeleteShader(vs);
    glDeleteShader(fs);
    return p;
}

// Programa para el modelo (con luz y textura)
inline GLuint crearPrograma() {
    return crearProgramaDesde(VERTEX_SHADER, FRAGMENT_SHADER);
}

// -----------------------------------------------------------------------------
//  SHADERS PARA LINEAS Y PUNTOS (curvas, ejes, puntos de control...)
//  Sin luz ni textura: solo un color plano.
// -----------------------------------------------------------------------------
inline const char* VERTEX_SHADER_LINEA = R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 mvp;
void main() {
    gl_Position = mvp * vec4(aPos, 1.0);
}
)";

inline const char* FRAGMENT_SHADER_LINEA = R"(#version 330 core
uniform vec3 color;
out vec4 FragColor;
void main() {
    FragColor = vec4(color, 1.0);
}
)";

inline GLuint crearProgramaLineas() {
    return crearProgramaDesde(VERTEX_SHADER_LINEA, FRAGMENT_SHADER_LINEA);
}