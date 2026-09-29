#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include "Lexer.h"
#include "TablaSimbolos.h"

int main() {
    std::string archivoPrueba = "../casos_de_prueba/prueba.lp";
    Lexer lexer(archivoPrueba);
    TablaSimbolos tablaSimbolos;
    Token token;
    std::vector<Token> listaTokens;

    // 1. Procesar tokens y poblar la tabla de símbolos
    do {
        token = lexer.getNextToken();
        if (token.tipo != FIN_ARCHIVO) {
            listaTokens.push_back(token);
            if (token.tipo == ID) {
                tablaSimbolos.insertar(token.lexema);
            }
        }
    } while (token.tipo != FIN_ARCHIVO);

    // Obtener los identificadores almacenados para imprimirlos al costado
    const auto& simbolos = tablaSimbolos.obtenerSimbolos();

    std::cout << "Iniciando analisis lexico...\n\n";

    // Encabezado unificado de dos columnas paralelas
    std::cout << std::string(85, '=') << "\n";
    std::cout << std::left
              << std::setw(20) << "LEXEMA"
              << std::setw(20) << "TOKEN"
              << " | "
              << std::setw(8)  << "POS"
              << std::setw(25) << "TABLA DE SIMBOLOS" << "\n";
    std::cout << std::string(85, '=') << "\n";

    // Calcular cuántas filas de salida se imprimirán en total
    size_t totalFilas = std::max(listaTokens.size(), simbolos.size());

    // 2. Imprimir ambos reportes renglón por renglón
    for (size_t i = 0; i < totalFilas; ++i) {
        // --- Columna de Tokens ---
        if (i < listaTokens.size()) {
            std::cout << std::left
                      << std::setw(20) << listaTokens[i].lexema
                      << std::setw(20) << tokenTypeToString(listaTokens[i].tipo);
        } else {
            std::cout << std::setw(40) << " ";
        }

        std::cout << " | ";

        // --- Columna de Tabla de Símbolos ---
        if (i < simbolos.size()) {
            std::cout << std::left
                      << std::setw(8)  << (i + 1)
                      << std::setw(25) << simbolos[i];
        }

        std::cout << "\n";
    }

    std::cout << std::string(85, '=') << "\n";

    std::cout << "\nPresiona Enter para salir...";
    std::cin.get();

    return 0;
}