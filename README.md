# Analizador Léxico - Fase 3 (Lenguaje LP)

## Integrantes
*Jose David Choquehuanca Calcina
*Jose Carlos Chino Nina

## Requisitos Previos
* Entorno Linux.
* Compilador GCC/G++ o similar.
* Herramienta `make` instalada.

## Características Implementadas
De acuerdo con los criterios de evaluación, este analizador incluye:
* **Generación de tokens:** Reconocimiento de tokens siguiendo estrictamente la nomenclatura de la segunda columna del documento de especificación del lenguaje LP.
* **Tabla de símbolos:** Almacenamiento correcto de identificadores y sus atributos correspondientes.
* **Manejo de errores:** Detección y reporte claro de errores encontrados por el analizador léxico.

## Estructura del Repositorio
* `src/` : Contiene el código fuente del analizador léxico (`main.cpp`, etc.).
* `casos_de_prueba/` : Directorio con los archivos de prueba.
* `CMakeLists.txt` : Archivo de configuración para compilar el proyecto con CMake.
* `README.md` : Instrucciones del proyecto.

## Requisitos Previos
Para compilar y ejecutar este proyecto en un entorno Linux, se requiere:
* Compilador C++ (GCC/G++ o Clang).
* CMake (versión 3.10 o superior).
* Herramienta `make`.

## Instrucciones de Compilación y Ejecución (Terminal Linux)
Este proyecto está configurado para utilizar CMake (compatible con CLion). Para que cualquier usuario pueda compilarlo desde la terminal sin necesidad de un IDE, siga estos pasos:

1. Abra una terminal y ubíquese en el directorio raíz del repositorio.
2. Cree un directorio para los binarios de compilación y acceda a él:
   ```bash
   mkdir build
   cd build
