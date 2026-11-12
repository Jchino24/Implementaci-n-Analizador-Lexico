#ifndef PARSER_H
#define PARSER_H

#include <cstddef>
#include <string>
#include <vector>

#include "Lexer.h"
#include "TablaSimbolos.h"
#include "Token.h"

// Tipo de problema detectado durante el análisis.
enum class TipoError {
    LEXICO,
    SINTACTICO
};

// Un error (o advertencia) con la posicion exacta dentro del archivo .lp.
struct ErrorCompilacion {
    TipoError tipo = TipoError::SINTACTICO;
    int linea = 0;
    int columna = 0;
    std::string mensaje;
    std::string pista;
};

// Analizador sintactico recursivo descendente para el lenguaje LP (Entregable 1).
//
// Recibe la ruta de un archivo .lp, obtiene la secuencia de tokens con el Lexer
// y reconoce la estructura:
//
//   PROGRAMA       -> TIPO_RETORNO PRINCIPAL BLOQUE
//   TIPO_RETORNO   -> void | TIPO_BASE
//   PRINCIPAL      -> main ( )
//   BLOQUE         -> { LISTA_SENTENCIAS }
//   SENTENCIA      -> DECLARACION | ASIGNACION
//   DECLARACION    -> TIPO LISTA_VARIABLES
//   TIPO           -> TIPO_BASE DIMENSION*
//   TIPO_BASE      -> int | float | char | boolean
//   DIMENSION      -> [ NUM_INT ]
//   LISTA_VARIABLES-> LISTA_ID ;
//   LISTA_ID       -> ID (, ID)*
//   ASIGNACION     -> ID ( [ COMPLEMENTO ] )* = COMPLEMENTO ;
//   COMPLEMENTO    -> EXPRESION [ COMP EXPRESION ]
//
// Ademas guarda una traza textual de las producciones reconocidas, pensada
// para las evidencias y la sustentacion.
class Parser {
public:
    explicit Parser(const std::string& rutaArchivo);

    // Ejecuta el analisis sintactico. Devuelve true solo si no hubo errores.
    bool analizar();

    bool archivoAbierto() const { return abierto; }
    const std::vector<Token>& obtenerTokens() const { return tokens; }
    const std::vector<ErrorCompilacion>& obtenerErrores() const { return errores; }
    const std::vector<ErrorCompilacion>& obtenerAdvertencias() const { return advertencias; }
    const std::vector<std::string>& obtenerTraza() const { return traza; }
    const std::vector<std::string>& obtenerLineas() const { return lineas; }
    const TablaSimbolos& obtenerTablaSimbolos() const { return tabla; }

    int totalDeclaraciones() const { return declaraciones; }
    int totalAsignaciones() const { return asignaciones; }
    int totalErroresLexicos() const;
    int totalErroresSintacticos() const;

private:
    // Informacion que produce TIPO y que luego se guarda en la tabla de simbolos.
    struct InfoTipo {
        std::string base;      // int, float, char, boolean
        std::string completo;  // int, float[10], int[3][3], ...
        int dimensiones = 0;
        bool valido = false;
    };

    // Cierra un nodo de la traza automaticamente al salir de una produccion.
    struct CierreNodo {
        Parser* parser;
        ~CierreNodo();
    };

    // --- Flujo de tokens -------------------------------------------------
    std::vector<Token> tokens;  // Todos los tokens, para el reporte lexico.
    std::vector<Token> flujo;   // Tokens utiles (sin DESCONOCIDO) + FIN_ARCHIVO.
    std::size_t pos = 0;

    std::vector<std::string> lineas;
    std::vector<ErrorCompilacion> errores;
    std::vector<ErrorCompilacion> advertencias;
    std::vector<std::string> traza;
    TablaSimbolos tabla;

    bool abierto = false;
    int nivel = 0;
    int declaraciones = 0;
    int asignaciones = 0;

    // --- Utilidades del flujo --------------------------------------------
    const Token& actual() const;
    bool esFin() const;
    void avanzar();
    bool comprobar(TokenType tipo) const;
    bool consumir(TokenType tipo);
    bool iniciaSentencia(TokenType tipo) const;
    static bool esTipoBase(TokenType tipo);
    static std::string describirToken(const Token& token);
    std::string textoEntre(std::size_t inicio, std::size_t fin) const;
    void sincronizar();

    // --- Errores ----------------------------------------------------------
    void reportar(TipoError tipo, int linea, int columna,
                  const std::string& mensaje, const std::string& pista = "");
    void errorActual(const std::string& esperado, const std::string& contexto,
                     const std::string& pista = "");
    void errorPuntoComa(const std::string& contexto);
    void advertencia(int linea, int columna, const std::string& mensaje,
                     const std::string& pista = "");

    // --- Traza ------------------------------------------------------------
    void abrirNodo(const std::string& etiqueta);
    void cerrarNodo();
    void nodo(const std::string& etiqueta);

    // --- Producciones de la gramatica ------------------------------------
    bool reconocerPrograma();
    bool reconocerPrincipal();
    bool reconocerBloque();
    bool reconocerDeclaracion();
    InfoTipo reconocerTipo();
    InfoTipo reconocerTipoBase(bool permitirVoid);
    bool reconocerDimension(InfoTipo& tipo);
    bool reconocerListaVariables(const InfoTipo& tipo);
    std::vector<std::string> reconocerListaId(const InfoTipo& tipo, bool& ok);
    bool reconocerAsignacion();
    bool reconocerSubindices();
    bool reconocerComplemento();
    bool reconocerExpresion();
    bool reconocerTermino();
    bool reconocerFactor();
};

#endif  // PARSER_H
