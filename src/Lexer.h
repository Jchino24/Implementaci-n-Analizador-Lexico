#ifndef LEXER_H
#define LEXER_H

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "Token.h"

// Problema detectado mientras se construyen los tokens.
struct ErrorLexico {
    int linea = 0;
    int columna = 0;
    std::string mensaje;
};

class Lexer {
private:
    std::ifstream archivo;
    int currentChar;
    int linea;
    int columna;
    std::vector<ErrorLexico> errores;
    std::unordered_map<std::string, TokenType> palabrasReservadas;

    void avanzar();
    void saltarEspaciosYComentarios();
    void registrarError(const std::string& mensaje, int lineaError, int columnaError);

public:
    explicit Lexer(const std::string& rutaArchivo);
    ~Lexer();

    bool estaAbierto() const;
    Token getNextToken();
    const std::vector<ErrorLexico>& obtenerErrores() const;
};

#endif  // LEXER_H
