#include "Lexer.h"

#include <cctype>
#include <iostream>
#include <string>

Lexer::Lexer(const std::string& rutaArchivo)
    : currentChar(EOF), linea(1), columna(0) {
    archivo.open(rutaArchivo);
    if (!archivo.is_open()) {
        std::cerr << "Error al abrir el archivo: " << rutaArchivo << std::endl;
        return;
    }

    // Mapa directo de palabras reservadas -> token.
    palabrasReservadas = {
        {"int", INT},       {"float", FLOAT}, {"char", CHAR},
        {"boolean", BOOLEAN}, {"void", VOID},  {"if", IF},
        {"else", ELSE},     {"for", FOR},     {"while", WHILE},
        {"scanf", SCANF},   {"println", PRINTLN}, {"main", MAIN},
        {"return", RETURN}};

    avanzar();
}

Lexer::~Lexer() {
    if (archivo.is_open()) {
        archivo.close();
    }
}

bool Lexer::estaAbierto() const {
    return archivo.is_open();
}

const std::vector<ErrorLexico>& Lexer::obtenerErrores() const {
    return errores;
}

void Lexer::registrarError(const std::string& mensaje, int lineaError,
                           int columnaError) {
    errores.push_back({lineaError, columnaError, mensaje});
}

void Lexer::avanzar() {
    const int siguiente = archivo.get();
    if (siguiente == EOF) {
        currentChar = EOF;
        return;
    }

    currentChar = siguiente;
    if (currentChar == '\n') {
        ++linea;
        columna = 0;
    } else {
        ++columna;
    }
}

void Lexer::saltarEspaciosYComentarios() {
    while (currentChar != EOF) {
        if (std::isspace(static_cast<unsigned char>(currentChar))) {
            avanzar();
        }
        // COMENT = //.*\n (comentarios de una línea)
        else if (currentChar == '/' && archivo.peek() == '/') {
            while (currentChar != EOF && currentChar != '\n') {
                avanzar();
            }
        } else {
            break;
        }
    }
}

Token Lexer::getNextToken() {
    saltarEspaciosYComentarios();

    if (currentChar == EOF) {
        return {FIN_ARCHIVO, "", linea, columna};
    }

    const int colInicio = columna;
    const int lineaInicio = linea;

    // 1. Identificadores (ID) y palabras reservadas.
    if (std::isalpha(static_cast<unsigned char>(currentChar)) || currentChar == '_' || currentChar == '$') {
        std::string lexema;
        while (std::isalnum(static_cast<unsigned char>(currentChar)) || currentChar == '_' ||
               currentChar == '$') {
            lexema += static_cast<char>(currentChar);
            avanzar();
        }

        const auto encontrada = palabrasReservadas.find(lexema);
        if (encontrada != palabrasReservadas.end()) {
            return {encontrada->second, lexema, lineaInicio, colInicio};
        }
        return {ID, lexema, lineaInicio, colInicio};
    }

    // 2. Constantes de texto ("...").
    if (currentChar == '"') {
        std::string lexema;
        avanzar();  // Consumir la comilla de apertura.

        while (currentChar != EOF && currentChar != '"' && currentChar != '\n') {
            lexema += static_cast<char>(currentChar);
            avanzar();
        }

        if (currentChar == '"') {
            avanzar();  // Consumir la comilla de cierre.
            return {TEXTO, lexema, lineaInicio, colInicio};
        }

        registrarError("constante de texto sin comilla de cierre", lineaInicio,
                       colInicio);
        return {DESCONOCIDO, lexema, lineaInicio, colInicio};
    }

    // 3. Números enteros y decimales.
    if (std::isdigit(static_cast<unsigned char>(currentChar))) {
        std::string lexema;
        bool esDecimal = false;

        while (std::isdigit(static_cast<unsigned char>(currentChar))) {
            lexema += static_cast<char>(currentChar);
            avanzar();
        }

        if (currentChar == '.') {
            const int siguiente = archivo.peek();
            if (siguiente != EOF && std::isdigit(static_cast<unsigned char>(siguiente))) {
                esDecimal = true;
                lexema += static_cast<char>(currentChar);
                avanzar();
                while (std::isdigit(static_cast<unsigned char>(currentChar))) {
                    lexema += static_cast<char>(currentChar);
                    avanzar();
                }
            }
        }

        return {esDecimal ? NUM_DEC : NUM_INT, lexema, lineaInicio, colInicio};
    }

    // 4. Operadores relacionales y de asignación.
    if (currentChar == '<' || currentChar == '>' || currentChar == '!' ||
        currentChar == '=') {
        std::string lexema;
        lexema += static_cast<char>(currentChar);
        const char opInicial = static_cast<char>(currentChar);
        avanzar();

        if (currentChar == '=') {  // <=, >=, !=, ==
            lexema += static_cast<char>(currentChar);
            avanzar();
            return {COMP, lexema, lineaInicio, colInicio};
        }

        if (opInicial == '<' || opInicial == '>') {
            return {COMP, lexema, lineaInicio, colInicio};
        }
        if (opInicial == '=') {
            return {ASIGNACION, lexema, lineaInicio, colInicio};
        }
        // opInicial == '!'
        return {NOT, lexema, lineaInicio, colInicio};
    }

    // 5. Operadores lógicos (&&, ||).
    if (currentChar == '&' && archivo.peek() == '&') {
        avanzar();
        avanzar();
        return {AND, "&&", lineaInicio, colInicio};
    }
    if (currentChar == '|' && archivo.peek() == '|') {
        avanzar();
        avanzar();
        return {OR, "||", lineaInicio, colInicio};
    }

    // 6. Operadores aritméticos y símbolos especiales simples.
    const char c = static_cast<char>(currentChar);
    const int lineaToken = linea;
    avanzar();
    switch (c) {
        case '+': return {SUMA, "+", lineaToken, colInicio};
        case '-': return {RESTA, "-", lineaToken, colInicio};
        case '*': return {MULT, "*", lineaToken, colInicio};
        case '/': return {DIV, "/", lineaToken, colInicio};
        case '%': return {MOD, "%", lineaToken, colInicio};
        case '(': return {PAR_IZQ, "(", lineaToken, colInicio};
        case ')': return {PAR_DER, ")", lineaToken, colInicio};
        case '[': return {CORCH_IZQ, "[", lineaToken, colInicio};
        case ']': return {CORCH_DER, "]", lineaToken, colInicio};
        case '{': return {LLAVE_IZQ, "{", lineaToken, colInicio};
        case '}': return {LLAVE_DER, "}", lineaToken, colInicio};
        case ',': return {COMA, ",", lineaToken, colInicio};
        case ';': return {PUNTO_COMA, ";", lineaToken, colInicio};
        case '.': return {PUNTO, ".", lineaToken, colInicio};
        default:
            registrarError("carácter no reconocido '" + std::string(1, c) + "'",
                           lineaToken, colInicio);
            return {DESCONOCIDO, std::string(1, c), lineaToken, colInicio};
    }
}
