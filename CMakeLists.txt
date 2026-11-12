cmake_minimum_required(VERSION 3.16)
project(AnalizadorLP LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ejecutable "analizador": integra lexer + parser.
add_executable(analizador
    src/main.cpp
    src/Lexer.cpp
    src/Token.cpp
    src/Parser.cpp
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(analizador PRIVATE -Wall -Wextra)
endif()
