#include "Lexer.h"
#include <cctype>
#include <iostream>

Lexer::Lexer(const std::string& rutaArchivo) {
    archivo.open(rutaArchivo);
    if (!archivo.is_open()) {
        std::cerr << "Error al abrir el archivo: " << rutaArchivo << std::endl;
    }
    linea = 1;
    columna = 0;

    // Mapa directo de Palabras Reservadas -> Su Token según la tabla
    palabrasReservadas = {
        {"int", INT},
        {"float", FLOAT},
        {"char", CHAR},
        {"boolean", BOOLEAN},
        {"void", VOID},
        {"if", IF},
        {"else", ELSE},
        {"for", FOR},
        {"while", WHILE},
        {"scanf", SCANF},
        {"println", PRINTLN},
        {"main", MAIN},
        {"return", RETURN}
    };

    avanzar();
}

Lexer::~Lexer() {
    if (archivo.is_open()) {
        archivo.close();
    }
}

void Lexer::avanzar() {
    if (archivo.get(currentChar)) {
        columna++;
        if (currentChar == '\n') {
            linea++;
            columna = 0;
        }
    } else {
        currentChar = EOF;
    }
}

void Lexer::saltarEspaciosYComentarios() {
    while (currentChar != EOF) {
        if (isspace(currentChar)) {
            avanzar();
        } 
        // COMENT = //.*\n (Manejo de comentarios de una línea)
        else if (currentChar == '/' && archivo.peek() == '/') {
            while (currentChar != EOF && currentChar != '\n') {
                avanzar();
            }
        } 
        else {
            break;
        }
    }
}

Token Lexer::getNextToken() {
    saltarEspaciosYComentarios();

    if (currentChar == EOF) {
        return {FIN_ARCHIVO, "", linea, columna};
    }

    int colInicio = columna;

    // 1. Identificadores (ID) y Palabras Reservadas
    // <--- CAMBIO REALIZADO: Se agregó '|| currentChar == '$'' para reconocer variables como $precio --->
    if (isalpha(currentChar) || currentChar == '_' || currentChar == '$') {
        std::string lexema = "";
        while (isalnum(currentChar) || currentChar == '_' || currentChar == '$') {
            lexema += currentChar;
            avanzar();
        }

        auto it = palabrasReservadas.find(lexema);
        if (it != palabrasReservadas.end()) {
            return {it->second, lexema, linea, colInicio};
        }
        return {ID, lexema, linea, colInicio};
    }

    // 2. Constantes de texto ("...")
    if (currentChar == '"') {
        std::string lexema = "";
        avanzar(); // Consumir comilla de apertura
        
        // Se añade '&& currentChar != '\n'' para detener la lectura al acabar la línea
        while (currentChar != EOF && currentChar != '"' && currentChar != '\n') {
            lexema += currentChar;
            avanzar();
        }
        if (currentChar == '"') {
            avanzar(); // Consumir comilla de cierre
            return {TEXTO, lexema, linea, colInicio};
        }
        // Si termina la línea o el archivo sin cerrar comilla, reporta DESCONOCIDO solo para esa línea
        return {DESCONOCIDO, lexema, linea, colInicio};
    }

    // 3. Números Enteros y Decimales
    if (isdigit(currentChar)) {
        std::string lexema = "";
        bool esDecimal = false;

        while (isdigit(currentChar)) {
            lexema += currentChar;
            avanzar();
        }

        if (currentChar == '.') {
            char nextChar = archivo.peek();
            if (isdigit(nextChar)) {
                esDecimal = true;
                lexema += currentChar;
                avanzar();
                while (isdigit(currentChar)) {
                    lexema += currentChar;
                    avanzar();
                }
            }
        }

        return {esDecimal ? NUM_DEC : NUM_INT, lexema, linea, colInicio};
    }

    // 4. Operadores de Comparación (COMP)
    if (currentChar == '<' || currentChar == '>' || currentChar == '!' || currentChar == '=') {
        std::string lexema = "";
        lexema += currentChar;
        char opInicial = currentChar;
        avanzar();

        if (currentChar == '=') { // <=, >=, !=, ==
            lexema += currentChar;
            avanzar();
            return {COMP, lexema, linea, colInicio};
        }

        if (opInicial == '<' || opInicial == '>') {
            return {COMP, lexema, linea, colInicio}; // < o > son COMP
        }

        if (opInicial == '=') {
            return {ASIGNACION, lexema, linea, colInicio}; // = solo es Asignación
        }

        if (opInicial == '!') {
            return {NOT, lexema, linea, colInicio}; // ! solo es NOT
        }
    }

    // 5. Operadores Lógicos (&&, ||)
    if (currentChar == '&' && archivo.peek() == '&') {
        avanzar(); avanzar();
        return {AND, "&&", linea, colInicio};
    }
    if (currentChar == '|' && archivo.peek() == '|') {
        avanzar(); avanzar();
        return {OR, "||", linea, colInicio};
    }

    // 6. Operadores Aritméticos y Símbolos Especiales simples
    char c = currentChar;
    avanzar();
    switch (c) {
        case '+': return {SUMA, "+", linea, colInicio};
        case '-': return {RESTA, "-", linea, colInicio};
        case '*': return {MULT, "*", linea, colInicio};
        case '/': return {DIV, "/", linea, colInicio};
        case '%': return {MOD, "%", linea, colInicio};
        case '(': return {PAR_IZQ, "(", linea, colInicio};
        case ')': return {PAR_DER, ")", linea, colInicio};
        case '[': return {CORCH_IZQ, "[", linea, colInicio};
        case ']': return {CORCH_DER, "]", linea, colInicio};
        case '{': return {LLAVE_IZQ, "{", linea, colInicio};
        case '}': return {LLAVE_DER, "}", linea, colInicio};
        case ',': return {COMA, ",", linea, colInicio};
        case ';': return {PUNTO_COMA, ";", linea, colInicio};
        case '.': return {PUNTO, ".", linea, colInicio}; // <--- CAMBIO REALIZADO: Reconocimiento del símbolo '.'
        default:  return {DESCONOCIDO, std::string(1, c), linea, colInicio};
    }
}