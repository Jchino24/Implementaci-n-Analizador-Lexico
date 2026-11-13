#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "Parser.h"
#include "Token.h"

namespace {

void imprimirUso(const std::string& programa) {
    std::cout << "Uso: " << programa << " <archivo.lp>\n\n"
              << "Ejemplo:\n"
              << "  " << programa << " casos_de_prueba/valido_basico.lp\n";
}

std::string recortar(const std::string& texto, std::size_t maximo) {
    if (texto.size() <= maximo) {
        return texto;
    }
    return texto.substr(0, maximo - 3) + "...";
}

std::string expandirTabs(const std::string& linea) {
    std::string resultado;
    for (const char c : linea) {
        if (c == '\t') {
            resultado += "    ";
        } else {
            resultado += c;
        }
    }
    return resultado;
}

int columnaVisual(const std::string& linea, int columna) {
    int visual = 0;
    for (int i = 0; i < columna - 1 && i < static_cast<int>(linea.size()); ++i) {
        visual += linea[static_cast<std::size_t>(i)] == '\t' ? 4 : 1;
    }
    return visual;
}

void imprimirError(const ErrorCompilacion& error,
                   const std::vector<std::string>& lineas) {
    const std::string etiqueta =
        error.tipo == TipoError::LEXICO ? "[ERROR LEXICO]" : "[ERROR SINTACTICO]";

    std::cout << etiqueta << " Línea " << error.linea << ": "
              << error.mensaje << "\n";

    if (error.linea >= 1 &&
        error.linea <= static_cast<int>(lineas.size())) {
        const std::string& original =
            lineas[static_cast<std::size_t>(error.linea - 1)];
        std::cout << "   " << std::right << std::setw(4) << error.linea
                  << " | " << expandirTabs(original) << "\n";
        std::cout << "        | "
                  << std::string(columnaVisual(original, error.columna), ' ')
                  << "^\n";
    }

    if (!error.pista.empty()) {
        std::cout << "        Sugerencia: " << error.pista << "\n";
    }
    std::cout << "\n";
}

void imprimirTokens(const std::vector<Token>& tokens) {
    std::cout << "SECUENCIA DE TOKENS\n";
    std::cout << std::string(60, '-') << "\n";
    std::cout << std::left << std::setw(5) << "#" << std::setw(24) << "LEXEMA"
              << std::setw(16) << "TOKEN" << "LINEA\n";
    std::cout << std::string(60, '-') << "\n";

    std::size_t numero = 0;
    for (const Token& token : tokens) {
        if (token.tipo == FIN_ARCHIVO) {
            continue;
        }
        ++numero;
        std::string lexema = token.lexema.empty() ? "<vacio>" : token.lexema;
        std::cout << std::left << std::setw(5) << numero << std::setw(24)
                  << recortar(lexema, 22) << std::setw(16)
                  << tokenTypeToString(token.tipo) << token.linea << "\n";
    }
    std::cout << std::string(60, '-') << "\n";
    std::cout << "Total: " << numero << " tokens\n\n";
}

std::string archivoPorDefecto() {
    const std::vector<std::string> candidatos = {
        "casos_de_prueba/valido_basico.lp",
        "../casos_de_prueba/valido_basico.lp"};
    for (const std::string& candidato : candidatos) {
        std::ifstream archivo(candidato);
        if (archivo.good()) {
            return candidato;
        }
    }
    return "";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::string ruta;

    for (int i = 1; i < argc; ++i) {
        const std::string argumento = argv[i];
        if (argumento == "-h" || argumento == "--ayuda") {
            imprimirUso(argv[0]);
            return 0;
        }
        if (!argumento.empty() && argumento[0] == '-') {
            std::cerr << "Opción desconocida: " << argumento << "\n\n";
            imprimirUso(argv[0]);
            return 2;
        }
        if (!ruta.empty()) {
            std::cerr << "Error: solo se puede analizar un archivo .lp.\n\n";
            imprimirUso(argv[0]);
            return 2;
        }
        ruta = argumento;
    }

    if (ruta.empty()) {
        ruta = archivoPorDefecto();
    }
    if (ruta.empty()) {
        std::cerr << "Error: no se indicó un archivo .lp.\n\n";
        imprimirUso(argv[0]);
        return 2;
    }

    Parser parser(ruta);
    if (!parser.archivoAbierto()) {
        std::cerr << "Error: no se pudo abrir el archivo: " << ruta << "\n";
        return 2;
    }

    std::cout << std::string(60, '=') << "\n";
    std::cout << " ANALIZADOR LÉXICO Y SINTÁCTICO - LENGUAJE LP\n";
    std::cout << " Entregable 1: estructura, declaraciones y asignaciones\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "Archivo: " << ruta << "\n\n";

    const bool exito = parser.analizar();

    imprimirTokens(parser.obtenerTokens());

    std::cout << "DIAGNÓSTICO\n";
    std::cout << std::string(60, '-') << "\n";
    if (parser.obtenerErrores().empty()) {
        std::cout << "Sin errores sintácticos.\n\n";
    } else {
        for (const ErrorCompilacion& error : parser.obtenerErrores()) {
            imprimirError(error, parser.obtenerLineas());
        }
    }

    std::cout << std::string(60, '=') << "\n";
    if (exito) {
        std::cout << "RESULTADO FINAL: PROGRAMA VÁLIDO\n";
        return 0;
    }

    std::cout << "RESULTADO FINAL: PROGRAMA CON ERRORES\n";
    return 1;
}
