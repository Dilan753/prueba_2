#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cstdint>
#include <vector>

// -----------------------------------------------------------------------------
//  Una "pieza" dibujable (un primitivo de glTF)
//  Un modelo suele tener muchas piezas (cuerpo, ruedas, ventanas...).
// -----------------------------------------------------------------------------
struct Pieza {
    GLuint vao = 0;            // formato de los datos en la GPU
    GLsizei numIndices = 0;    // cuantos indices dibujar
    GLuint textura = 0;        // 0 = sin textura
    glm::vec4 color{ 1.0f };   // color base del material
    glm::mat4 modelo{ 1.0f };  // transformacion del nodo al que pertenece
    bool prota = false;        // true = pertenece a la protagonista (se mueve en la animacion)
};

// -----------------------------------------------------------------------------
//  Sube los vertices e indices a la GPU y los deja listos en pz.vao
//
//  datos:   [x y z  nx ny nz  u v] por cada vertice (8 floats)
//  indices: que vertices forman cada triangulo
// -----------------------------------------------------------------------------
inline void subirMalla(Pieza& pz,
    const std::vector<float>& datos,
    const std::vector<uint32_t>& indices) {
    glGenVertexArrays(1, &pz.vao);
    glBindVertexArray(pz.vao);

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, datos.size() * sizeof(float), datos.data(), GL_STATIC_DRAW);


    GLsizei stride = 8 * sizeof(float);
    glEnableVertexAttribArray(0);  // posicion
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(1);  // normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);  // uv
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));

    GLuint ebo;
    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t),
        indices.data(), GL_STATIC_DRAW);

    pz.numIndices = (GLsizei)indices.size();
    glBindVertexArray(0);
}

// -----------------------------------------------------------------------------
//  Dibuja una pieza (el shader ya debe estar activo con view y proj enviados)
//  base = transformacion extra que se aplica antes de la del nodo
// -----------------------------------------------------------------------------
inline void dibujarPieza(GLuint shader, const Pieza& p, const glm::mat4& base) {
    glm::mat4 modelo = base * p.modelo;
    glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(modelo));
    glUniform4fv(glGetUniformLocation(shader, "baseColor"), 1, glm::value_ptr(p.color));
    glUniform1i(glGetUniformLocation(shader, "hasTex"), p.textura != 0);

    if (p.textura) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, p.textura);
    }
    glBindVertexArray(p.vao);
    glDrawElements(GL_TRIANGLES, p.numIndices, GL_UNSIGNED_INT, 0);
}