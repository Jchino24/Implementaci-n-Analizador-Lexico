#ifndef TOKEN_H
#define TOKEN_H

#include <string>

enum TokenType {
    // Tipos Generales
    NUM_INT,
    NUM_DEC,
    ID,
    TEXTO,

    // Palabras Reservadas (Tokens individuales)
    INT,
    FLOAT,
    CHAR,
    BOOLEAN,
    VOID,
    IF,
    ELSE,
    FOR,
    WHILE,
    SCANF,
    PRINTLN,
    MAIN,
    RETURN,

    // Operadores
    ASIGNACION,   // =
    SUMA,         // +
    RESTA,        // -
    MULT,         // *
    DIV,          // /
    MOD,          // %
    AND,          // &&
    OR,           // ||
    NOT,          // !
    COMP,         // > | >= | < | <= | != | ==

    // Símbolos Especiales
    PAR_IZQ,      // (
    PAR_DER,      // )
    CORCH_IZQ,    // [
    CORCH_DER,    // ]
    LLAVE_IZQ,    // {
    LLAVE_DER,    // }
    COMA,         // ,
    PUNTO_COMA,   // ;
    PUNTO,        // .

    DESCONOCIDO,
    FIN_ARCHIVO
};

struct Token {
    TokenType tipo;
    std::string lexema;
    int linea;
    int columna;
};

std::string tokenTypeToString(TokenType tipo);

#endif