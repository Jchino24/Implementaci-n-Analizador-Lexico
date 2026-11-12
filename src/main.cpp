#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "Parser.h"
#include "Token.h"

namespace {

const std::string kSeparador(78, '=');
const std::string kSubseparador(78, '-');

void imprimirUso(const std::string& programa) {
    std::cout
        << "Uso: " << programa << " <archivo.lp> [opciones]\n\n"
        << "Opciones:\n"
        << "  --sin-tokens   Omite la tabla de tokens.\n"
        << "  --sin-traza    Omite la traza de reconocimiento sintáctico.\n"
        << "  --sin-tabla    Omite la tabla de símbolos.\n"
        << "  -h, --ayuda    Muestra esta ayuda.\n\n"
        << "Ejemplo:\n"
        << "  " << programa << " casos_de_prueba/valido_basico.lp\n\n"
        << "Códigos de salida: 0 = programa válido, 1 = errores, "
           "2 = error de uso o de archivo.\n";
}

std::string recortar(const std::string& texto, std::size_t maximo) {
    if (texto.size() <= maximo) {
        return texto;
    }
    if (maximo <= 3) {
        return texto.substr(0, maximo);
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
    const int limite = columna - 1;
    for (int i = 0; i < limite && i < static_cast<int>(linea.size()); ++i) {
        if (linea[static_cast<std::size_t>(i)] == '\t') {
            visual += 4;
        } else {
            ++visual;
        }
    }
    return visual;
}

void imprimirError(const ErrorCompilacion& error, bool esAdvertencia,
                   const std::vector<std::string>& lineas) {
    std::string etiqueta;
    if (esAdvertencia) {
        etiqueta = "[ADVERTENCIA]";
    } else if (error.tipo == TipoError::LEXICO) {
        etiqueta = "[ERROR LEXICO]";
    } else {
        etiqueta = "[ERROR SINTACTICO]";
    }

    std::cout << etiqueta << " Línea " << error.linea << ": " << error.mensaje
              << "\n";

    if (error.linea >= 1 &&
        error.linea <= static_cast<int>(lineas.size())) {
        const std::string& lineaOriginal =
            lineas[static_cast<std::size_t>(error.linea - 1)];
        std::cout << "   " << std::right << std::setw(4) << error.linea
                  << " | " << expandirTabs(lineaOriginal) << "\n";
        std::cout << "        | "
                  << std::string(columnaVisual(lineaOriginal, error.columna), ' ')
                  << "^\n";
    }

    if (!error.pista.empty()) {
        std::cout << "        Sugerencia: " << error.pista << "\n";
    }
    std::cout << "\n";
}

void imprimirTokens(const std::vector<Token>& tokens) {
    std::cout << "SECUENCIA DE TOKENS (ANÁLISIS LÉXICO)\n";
    std::cout << kSubseparador << "\n";
    std::cout << std::left << std::setw(5) << "#" << std::setw(24) << "LEXEMA"
              << std::setw(16) << "TOKEN" << "LINEA\n";
    std::cout << kSubseparador << "\n";

    std::size_t numero = 0;
    for (const Token& token : tokens) {
        if (token.tipo == FIN_ARCHIVO) {
            continue;
        }
        ++numero;

        std::string lexema = token.lexema;
        if (lexema.empty()) {
            lexema = "<vacio>";
        }

        std::cout << std::left << std::setw(5) << numero << std::setw(24)
                  << recortar(lexema, 22) << std::setw(16)
                  << tokenTypeToString(token.tipo) << token.linea << "\n";
    }

    std::cout << kSubseparador << "\n";
    std::cout << "Total: " << numero << " tokens\n\n";
}

void imprimirTraza(const std::vector<std::string>& traza) {
    std::cout << "TRAZA DE RECONOCIMIENTO SINTÁCTICO\n";
    std::cout << kSubseparador << "\n";
    if (traza.empty()) {
        std::cout << "(sin traza)\n";
    } else {
        for (const std::string& linea : traza) {
            std::cout << linea << "\n";
        }
    }
    std::cout << "\n";
}

void imprimirTabla(const TablaSimbolos& tabla) {
    std::cout << "TABLA DE SÍMBOLOS (VARIABLES DECLARADAS)\n";
    std::cout << kSubseparador << "\n";

    const std::vector<Simbolo>& simbolos = tabla.obtenerSimbolos();
    if (simbolos.empty()) {
        std::cout << "(no se registraron declaraciones)\n\n";
        return;
    }

    std::cout << std::left << std::setw(5) << "#" << std::setw(20) << "NOMBRE"
              << std::setw(20) << "TIPO" << "LINEA\n";
    std::cout << kSubseparador << "\n";
    std::size_t numero = 0;
    for (const Simbolo& simbolo : simbolos) {
        ++numero;
        std::cout << std::left << std::setw(5) << numero << std::setw(20)
                  << recortar(simbolo.nombre, 18) << std::setw(20)
                  << recortar(simbolo.tipo, 18) << simbolo.linea << "\n";
    }
    std::cout << "\n";
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
    bool mostrarTokens = true;
    bool mostrarTraza = true;
    bool mostrarTabla = true;
    std::vector<std::string> archivos;

    for (int i = 1; i < argc; ++i) {
        const std::string argumento = argv[i];
        if (argumento == "-h" || argumento == "--ayuda") {
            imprimirUso(argv[0]);
            return 0;
        }
        if (argumento == "--sin-tokens") {
            mostrarTokens = false;
        } else if (argumento == "--sin-traza") {
            mostrarTraza = false;
        } else if (argumento == "--sin-tabla") {
            mostrarTabla = false;
        } else if (!argumento.empty() && argumento[0] == '-') {
            std::cerr << "Opción desconocida: " << argumento << "\n\n";
            imprimirUso(argv[0]);
            return 2;
        } else {
            archivos.push_back(argumento);
        }
    }

    if (archivos.size() > 1) {
        std::cerr << "Error: solo se puede analizar un archivo .lp a la vez.\n\n";
        imprimirUso(argv[0]);
        return 2;
    }

    std::string ruta = archivos.empty() ? archivoPorDefecto() : archivos.front();
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

    std::cout << kSeparador << "\n";
    std::cout << " ANALIZADOR LÉXICO Y SINTÁCTICO - LENGUAJE LP\n";
    std::cout << " Entregable 1: integración lexer + parser\n";
    std::cout << kSeparador << "\n";
    std::cout << "Archivo: " << ruta << "\n\n";

    const bool exito = parser.analizar();

    if (mostrarTokens) {
        imprimirTokens(parser.obtenerTokens());
    }
    if (mostrarTraza) {
        imprimirTraza(parser.obtenerTraza());
    }
    if (mostrarTabla) {
        imprimirTabla(parser.obtenerTablaSimbolos());
    }

    std::cout << "DIAGNÓSTICO\n";
    std::cout << kSubseparador << "\n";

    const std::vector<ErrorCompilacion>& errores = parser.obtenerErrores();
    const std::vector<ErrorCompilacion>& advertencias =
        parser.obtenerAdvertencias();

    if (errores.empty() && advertencias.empty()) {
        std::cout << "Sin errores ni advertencias.\n\n";
    } else {
        for (const ErrorCompilacion& error : errores) {
            imprimirError(error, false, parser.obtenerLineas());
        }
        for (const ErrorCompilacion& aviso : advertencias) {
            imprimirError(aviso, true, parser.obtenerLineas());
        }
    }

    std::cout << "RESULTADO\n";
    std::cout << kSubseparador << "\n";
    std::cout << std::left << std::setw(32) << "Declaraciones reconocidas:"
              << parser.totalDeclaraciones() << "\n";
    std::cout << std::left << std::setw(32) << "Asignaciones reconocidas:"
              << parser.totalAsignaciones() << "\n";
    std::cout << std::left << std::setw(32) << "Errores léxicos:"
              << parser.totalErroresLexicos() << "\n";
    std::cout << std::left << std::setw(32) << "Errores sintácticos:"
              << parser.totalErroresSintacticos() << "\n";
    std::cout << std::left << std::setw(32) << "Advertencias:"
              << advertencias.size() << "\n";
    std::cout << kSeparador << "\n";

    if (exito) {
        std::cout << "RESULTADO FINAL: PROGRAMA VÁLIDO\n";
        return 0;
    }

    std::cout << "RESULTADO FINAL: PROGRAMA CON ERRORES\n";
    return 1;
}
