#ifndef LEXER_H
#define LEXER_H

#include <fstream>
#include <string>
#include <unordered_map>
#include "Token.h"

class Lexer {
private:
    std::ifstream archivo;
    char currentChar;
    int linea;
    int columna;

    std::unordered_map<std::string, TokenType> palabrasReservadas;

    void avanzar();
    void saltarEspaciosYComentarios();

public:
    Lexer(const std::string& rutaArchivo);
    ~Lexer();
    Token getNextToken();
};

#endif