
**Autor:** Dilan Andrés Rodríguez Juyo
**Materia:** Computación Gráfica

El proyectó carga un modelo glTF, lo ilumina con el modelo de **Phong** y hace que una de sus piezas recorra una **curva de Bézier** con una cámara en tercera persona. Todo se controla desde una interfaz hecha con **Dear ImGui**.

## Qué hace

- **Carga de glTF:** lee el modelo *Gears of War Custom Lancer* (nodos, mallas, texturas y transformaciones jerárquicas) con tinygltf.
- **Iluminación Phong:** luz puntual con componente ambiente, difusa y especular, con atenuación.
- **Curva de Bézier** se dibuja con su polígono y sus puntos de control, y el arma la recorre.
- **Animación:** el arma se desplaza por la curva mientras gira, y la cámara la sigue.
- **Expositor:** todo el modelo gira sobre sí mismo, como en una vitrina.
- **Luz controlable:** posición, color e intensidad desde la interfaz.
- **Cámara libre** con el teclado.

## Controles

**Interfaz **

| Control | Qué hace |
|---|---|
| **Iniciar / Detener animación** | Arranca o cancela el recorrido del arma por la curva (cámara en tercera persona). |
| **Girar modelo** + **Velocidad** | Activa el modo expositor. La velocidad puede ser negativa para girar al otro lado. Se bloquea mientras corre la animación. |
| **Posición** (X/Y/Z) | Mueve la luz. |
| **Color** | Cambia el color de la luz. |
| **Intensidad** | Sube o baja la fuerza de la luz. |
| **Reiniciar luz** | Devuelve la luz a su estado inicial. |

**Cámara libre (teclado)**

| Tecla | Acción |
|---|---|
| `W` / `S` | Adelante / atrás |
| `A` / `D` | Izquierda / derecha |
| `E` / `Q` | Subir / bajar |
| `Esc` | Cerrar el programa |

> Durante la animación la cámara sigue al arma automáticamente y las teclas de movimiento se desactivan.

## Requisitos

- **Windows** de 64 bits
- **Visual Studio 2022 o superior** con el componente *Desarrollo para el escritorio con C++*
- Una tarjeta gráfica compatible con **OpenGL 3.3**
- La carpeta **`OpenGL`** incluida en este repositorio

### Librerías (incluidas en la carpeta `OpenGL`)

| Librería | Para qué se usa |
|---|---|
| **GLFW 3.4** (`lib-vc2022`) | Ventana, contexto OpenGL y entrada |
| **GLAD** | Carga de las funciones de OpenGL |
| **GLM** | Matemáticas: vectores y matrices |
| **stb** (`stb_image`) | Lectura de imágenes para las texturas |
| **tinygltf** (en `attic`) | Lectura de archivos glTF |
| **Dear ImGui** (con backends GLFW y OpenGL3) | Interfaz gráfica |

---

## Instalación

El proyecto busca las librerías en la ruta fija **`C:\OpenGL`**. Si se siguen estos pasos, **no hay que modificar nada dentro de Visual Studio**.

1. **Clonar o descargar** este repositorio.
2. **Copiar la carpeta `OpenGL`** del repositorio a la raíz del disco local, de modo que quede exactamente así:

   ```
   C:\OpenGL\glad
   C:\OpenGL\glfw-3.4.bin.WIN64
   C:\OpenGL\glm
   C:\OpenGL\stb-master
   C:\OpenGL\attic
   C:\OpenGL\imgui
   ```

3. Abrir `proyrcto corte 3 finanzacion.slnx` con Visual Studio.
4. Seleccionar la configuración **x64** (Debug o Release).
5. Presionar **F5** (o *Depurar → Iniciar depuración*).

El modelo se carga con una ruta relativa (`gears_of_war_custom_lancer/scene.gltf`), así que el programa debe ejecutarse con el directorio de trabajo en la carpeta del proyecto. Visual Studio lo hace por defecto con F5.

### Qué hace cada archivo

| Archivo | Responsabilidad |
|---|---|
| `proyrcto corte 3 finanzacion.cpp` | **Punto de entrada.** Crea la ventana y el contexto OpenGL, inicia ImGui, construye la interfaz, calcula las matrices de cada cuadro y llama a dibujar. |
| `Shaders.h` | Código GLSL (vertex y fragment) con iluminación Phong, shaders simples para líneas y puntos, y funciones para compilarlos y enlazarlos. |
| `Buffers.h` | Estructura `Pieza` (un primitivo del glTF) y la subida de vértices e índices a la GPU (VAO, VBO, EBO). Dibuja una pieza. |
| `Modelo.h` | Lee el `.gltf` con tinygltf, sube texturas y mallas, recorre el árbol de nodos multiplicando la transformación del padre por la del hijo, y marca qué piezas son la "protagonista" (el arma). |
| `Camara.h` | Matriz de vista (rotación × traslación). Cámara libre con teclado y cámara en tercera persona que sigue al arma. |
| `Bezier.h` | Curva de Bézier `P(t) = (1-t)³P0 + 3(1-t)²t P1 + 3(1-t)t² P2 + t³P3`. Calcula puntos y dibuja la curva, el polígono y los puntos de control. |
| `LUZ.h` | Luz puntual: posición, color e intensidad, y el marcador naranja que muestra dónde está. |
| `animacion.h` | Controla el progreso `t` (0 a 1) del recorrido y su duración. |

### Flujo de cada cuadro

1. ImGui construye la interfaz y lee los controles.
2. Se actualiza la animación (`t`) y el ángulo del expositor con el tiempo entre cuadros (`dt`).
3. Se calculan las matrices: **vista** (libre o tercera persona), **proyección** (perspectiva) y **modelo**.
4. Durante la animación, el arma se transforma así: centrarse en el origen → girar sobre su eje → colocarse sobre la curva de Bézier. El resto del modelo se queda quieto.
5. Se dibujan el modelo (Phong), la curva, el punto de luz y, al final, la interfaz.

---

## Estructura del repositorio

```
OpenGL/                          Librerías (copiar a C:\OpenGL)
proyrcto corte 3 finanzacion/
  ├─ proyrcto corte 3 finanzacion.cpp
  ├─ Shaders.h  Buffers.h  Modelo.h
  ├─ Camara.h   Bezier.h   LUZ.h   animacion.h
  └─ gears_of_war_custom_lancer/   Modelo glTF y texturas
proyrcto corte 3 finanzacion.slnx
```

## Capturas



Créditos
- Modelo **"Gears of war custom lancer"** de [Chenchanchong](https://sketchfab.com/Chenchanchong), disponible en [Sketchfab](https://sketchfab.com/3d-models/gears-of-war-custom-lancer-3c52fcfea5de41e190007f4e43c7fd92), con licencia [CC-BY-4.0](http://creativecommons.org/licenses/by/4.0/).
- Librerías: [GLFW](https://www.glfw.org/), [GLAD](https://glad.dav1d.de/), [GLM](https://github.com/g-truc/glm), [stb](https://github.com/nothings/stb), [tinygltf](https://github.com/syoyo/tinygltf) y [Dear ImGui](https://github.com/ocornut/imgui).
