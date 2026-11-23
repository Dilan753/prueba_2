// =============================================================================
//  FLUJO
//    1. tinygltf lee el .gltf (+ .bin + imagenes) y lo deja en RAM.
//    2. Subimos texturas, vertices e indices a la GPU.
//    3. Guardamos todo en una lista de Piezas lista para dibujar.
// =============================================================================
#pragma once

#include <glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include "tiny_gltf.h"
#include "buffers.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

class Modelo {
public:
    // Carga el archivo (necesita el contexto OpenGL ya creado)
    // nodosProta = indices de los nodos glTF 
    // Lista vacia = todo el modelo se mueve junto y se imprime la lista de nodos en consola.
    bool cargar(const std::string& ruta, const std::vector<int>& nodosProta = {}) {
        nodosProtagonista = nodosProta;
        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string err, warn;

        // Lee el .gltf, el .bin y decodifica las imagenes (rutas relativas al .gltf)
        if (!loader.LoadASCIIFromFile(&model, &err, &warn, ruta)) {
            std::cerr << "Error cargando glTF: " << err << "\n";
            return false;
        }
        if (!warn.empty()) std::cout << "Aviso: " << warn << "\n";

        // Primero las texturas (las mallas las referencian por indice)
        texturas.assign(model.textures.size(), 0);
        for (size_t i = 0; i < model.textures.size(); i++) {
            int src = model.textures[i].source;
            if (src >= 0) texturas[i] = crearTextura(model.images[src]);
        }

        // Luego la escena: recorrer los nodos raiz
        int escena = model.defaultScene >= 0 ? model.defaultScene : 0;

        // Si aun no hay protagonista, mostrar los nodos para poder elegirla
        if (nodosProta.empty()) {
            std::cout << "--- Nodos del modelo (elige uno como protagonista) ---\n";
            for (int nodo : model.scenes[escena].nodes)
                imprimirNodo(model, nodo, 0);
            std::cout << "------------------------------------------------------\n";
        }

        for (int nodo : model.scenes[escena].nodes)
            procesarNodo(model, nodo, glm::mat4(1.0f), false);

        std::cout << "Cargado: " << piezas.size() << " piezas, "
            << texturas.size() << " texturas\n";
        return !piezas.empty();
    }

    // Matriz que centra el modelo en el origen y lo escala a ~1 de radio.
    // Los modelos de Sketchfab vienen en escalas muy distintas, por eso esto.
    glm::mat4 normalizar() const {
        glm::vec3 centro = (boundsMin + boundsMax) * 0.5f;
        float radio = glm::length(boundsMax - boundsMin) * 0.5f;
        return glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / radio)) *
            glm::translate(glm::mat4(1.0f), -centro);
    }

    // Centro de la protagonista ya transformado con "mundo"
    // (si no hay protagonista, el centro de todo el modelo)
    glm::vec3 centroProta(const glm::mat4& mundo) const {
        glm::vec3 mn = hayProta ? protaMin : boundsMin;
        glm::vec3 mx = hayProta ? protaMax : boundsMax;
        return glm::vec3(mundo * glm::vec4((mn + mx) * 0.5f, 1.0f));
    }

    //   base      = transformacion de las piezas normales
    //   baseProta = transformacion de las piezas de la protagonista
    void dibujar(GLuint shader, const glm::mat4& base, const glm::mat4& baseProta) const {
        for (const Pieza& p : piezas)
            dibujarPieza(shader, p, p.prota ? baseProta : base);
    }

private:
    std::vector<Pieza>  piezas;     
    std::vector<GLuint> texturas;   
    glm::vec3 boundsMin{ 1e30f }, boundsMax{ -1e30f };  

    std::vector<int> nodosProtagonista;                  
    bool hayProta = false;                               
    glm::vec3 protaMin{ 1e30f }, protaMax{ -1e30f };     

    // --- subir una imagen a la GPU como textura ---
    static GLuint crearTextura(const tinygltf::Image& img) {
        if (img.image.empty()) return 0;

        GLenum formato = GL_RGBA;
        if (img.component == 1) formato = GL_RED;
        if (img.component == 2) formato = GL_RG;
        if (img.component == 3) formato = GL_RGB;

        GLuint id;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, formato, img.width, img.height, 0,
            formato, GL_UNSIGNED_BYTE, img.image.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        return id;
    }

    // --- leer datos de un accessor de glTF (accessor -> bufferView -> buffer) ---
    static std::vector<float> leerFloats(const tinygltf::Model& m, int idxAccessor, int comps) {
        const tinygltf::Accessor& acc = m.accessors[idxAccessor];
        const tinygltf::BufferView& bv = m.bufferViews[acc.bufferView];
        const tinygltf::Buffer& buf = m.buffers[bv.buffer];

        const unsigned char* p = buf.data.data() + bv.byteOffset + acc.byteOffset;
        int stride = acc.ByteStride(bv);  
        if (stride <= 0) stride = sizeof(float) * comps;

        std::vector<float> out(acc.count * comps);
        for (size_t i = 0; i < acc.count; i++)
            std::memcpy(&out[i * comps], p + i * stride, sizeof(float) * comps);
        return out;
    }

    // convertimos todos a 32.
    static std::vector<uint32_t> leerIndices(const tinygltf::Model& m, int idxAccessor) {
        const tinygltf::Accessor& acc = m.accessors[idxAccessor];
        const tinygltf::BufferView& bv = m.bufferViews[acc.bufferView];
        const tinygltf::Buffer& buf = m.buffers[bv.buffer];
        const unsigned char* p = buf.data.data() + bv.byteOffset + acc.byteOffset;

        std::vector<uint32_t> out(acc.count);
        for (size_t i = 0; i < acc.count; i++) {
            if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                out[i] = p[i];
            else if (acc.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                out[i] = ((const uint16_t*)p)[i];
            else
                out[i] = ((const uint32_t*)p)[i];
        }
        return out;
    }

    // --- convertir un primitivo de glTF en una Pieza lista para dibujar ---
    void crearPieza(const tinygltf::Model& m, const tinygltf::Primitive& prim,
        const glm::mat4& mundo, bool enProta) {
        // Solo manejamos triangulos con posiciones e indices
        if (prim.mode != TINYGLTF_MODE_TRIANGLES) return;
        if (!prim.attributes.count("POSITION")) return;
        if (prim.indices < 0) {
            std::cerr << "Primitivo sin indices, se omite en este ejemplo\n";
            return;
        }

        // 1) Leer atributos de la RAM
        std::vector<float> pos = leerFloats(m, prim.attributes.at("POSITION"), 3);
        std::vector<float> nor, uv;
        if (prim.attributes.count("NORMAL"))     nor = leerFloats(m, prim.attributes.at("NORMAL"), 3);
        if (prim.attributes.count("TEXCOORD_0")) uv = leerFloats(m, prim.attributes.at("TEXCOORD_0"), 2);

        // 2) Intercalar en un solo arreglo: [x y z  nx ny nz  u v] por vertice
        size_t n = pos.size() / 3;
        std::vector<float> datos(n * 8);
        for (size_t i = 0; i < n; i++) {
            float* d = &datos[i * 8];
            d[0] = pos[i * 3]; d[1] = pos[i * 3 + 1]; d[2] = pos[i * 3 + 2];
            d[3] = nor.empty() ? 0.0f : nor[i * 3];
            d[4] = nor.empty() ? 1.0f : nor[i * 3 + 1];
            d[5] = nor.empty() ? 0.0f : nor[i * 3 + 2];
            d[6] = uv.empty() ? 0.0f : uv[i * 2];
            d[7] = uv.empty() ? 0.0f : uv[i * 2 + 1];

            // actualizar la caja que envuelve al modelo (para centrarlo luego)
            glm::vec3 w = glm::vec3(mundo * glm::vec4(d[0], d[1], d[2], 1.0f));
            boundsMin = glm::min(boundsMin, w);
            boundsMax = glm::max(boundsMax, w);
            if (enProta) {
                protaMin = glm::min(protaMin, w);
                protaMax = glm::max(protaMax, w);
            }
        }

        // 3) Subir a la GPU (VAO + VBO + EBO)
        Pieza pz;
        pz.modelo = mundo;
        pz.prota = enProta;
        if (enProta) hayProta = true;
        subirMalla(pz, datos, leerIndices(m, prim.indices));

        // 4) Material: color base y textura
        if (prim.material >= 0) {
            const auto& pbr = m.materials[prim.material].pbrMetallicRoughness;
            if (pbr.baseColorFactor.size() == 4)
                pz.color = glm::vec4(pbr.baseColorFactor[0], pbr.baseColorFactor[1],
                    pbr.baseColorFactor[2], pbr.baseColorFactor[3]);
            int t = pbr.baseColorTexture.index;   
            if (t >= 0 && t < (int)texturas.size()) pz.textura = texturas[t];
        }

        piezas.push_back(pz);
    }

    // --- recorrer los nodos (arbol de transformaciones) ---
    // La transformacion de un hijo se multiplica por la de su padre.
    // dentroProta = true si algun ancestro de este nodo es la protagonista
    void procesarNodo(const tinygltf::Model& m, int idx, const glm::mat4& padre, bool dentroProta) {
        const tinygltf::Node& nodo = m.nodes[idx];

        glm::mat4 local(1.0f);
        if (nodo.matrix.size() == 16) {
            local = glm::mat4(glm::make_mat4(nodo.matrix.data()));   // viene como double
        }
        else {
            if (nodo.translation.size() == 3)
                local = glm::translate(local, glm::vec3(nodo.translation[0], nodo.translation[1], nodo.translation[2]));
            if (nodo.rotation.size() == 4)   // glTF: x,y,z,w   glm::quat(w,x,y,z)
                local *= glm::mat4_cast(glm::quat((float)nodo.rotation[3], (float)nodo.rotation[0],
                    (float)nodo.rotation[1], (float)nodo.rotation[2]));
            if (nodo.scale.size() == 3)
                local = glm::scale(local, glm::vec3(nodo.scale[0], nodo.scale[1], nodo.scale[2]));
        }
        glm::mat4 mundo = padre * local;
        // con la lista vacia todo el modelo cuenta como protagonista (se mueve junto)
        bool esElegido = std::find(nodosProtagonista.begin(), nodosProtagonista.end(), idx)
            != nodosProtagonista.end();
        bool enProta = dentroProta || esElegido || nodosProtagonista.empty();

        if (nodo.mesh >= 0)
            for (const auto& prim : m.meshes[nodo.mesh].primitives)
                crearPieza(m, prim, mundo, enProta);

        for (int hijo : nodo.children)
            procesarNodo(m, hijo, mundo, enProta);
    }

    // --- ayuda para elegir la protagonista: lista de nodos en consola ---

    // cuantos primitivos hay en un nodo y en todos sus hijos
    static int contarPiezas(const tinygltf::Model& m, int idx) {
        const tinygltf::Node& nodo = m.nodes[idx];
        int n = 0;
        if (nodo.mesh >= 0) n += (int)m.meshes[nodo.mesh].primitives.size();
        for (int hijo : nodo.children) n += contarPiezas(m, hijo);
        return n;
    }

    // imprime "[indice] nombre (N piezas)" con sangria segun la profundidad
    static void imprimirNodo(const tinygltf::Model& m, int idx, int nivel) {
        int n = contarPiezas(m, idx);
        if (n == 0 || nivel > 6) return;
        std::cout << std::string(nivel * 2, ' ') << "[" << idx << "] "
            << m.nodes[idx].name << "  (" << n << " piezas)\n";
        for (int hijo : m.nodes[idx].children)
            imprimirNodo(m, hijo, nivel + 1);
    }
};