cmake_minimum_required(VERSION 4.4)
project(AnalizadorLexico)

set(CMAKE_CXX_STANDARD 20)

# Aquí le decimos que el ejecutable se llamará "analizador"
# y que el código fuente está dentro de la carpeta src/
add_executable(analizador src/main.cpp src/Lexer.cpp src/Token.cpp)