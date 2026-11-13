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

// Un error con la posicion exacta dentro del archivo .lp.
struct ErrorCompilacion {
    TipoError tipo = TipoError::SINTACTICO;
    int linea = 0;
    int columna = 0;
    std::string mensaje;
    std::string pista;
};

// Analizador sintactico recursivo descendente para el lenguaje LP.
//
// La gramatica implementada es la del documento "Gramatica del Lenguaje LP":
//
//   PROGRAMA                -> LISTA_FUNCIONES PRINCIPAL
//   LISTA_FUNCIONES         -> DECLARACION_FUNCION LISTA_FUNCIONES | epsilon
//   DECLARACION_FUNCION     -> TIPO_RETORNO id ( PARAMETROS ) BLOQUE
//   TIPO_RETORNO            -> TIPO | void
//   TIPO                    -> TIPO_BASE DIMENSION
//   TIPO_BASE               -> char | float | int | boolean
//   DIMENSION               -> [ num_int ] DIMENSION | epsilon
//   PARAMETROS              -> TIPO id LISTA_PARAMETROS | epsilon
//   LISTA_PARAMETROS        -> , TIPO id LISTA_PARAMETROS | epsilon
//   PRINCIPAL               -> main ( ) BLOQUE
//                              (se acepta void main() por compatibilidad)
//   BLOQUE                  -> { LISTA_VARIABLES COMANDOS }
//   LISTA_VARIABLES         -> TIPO id LISTA_ID ; LISTA_VARIABLES | epsilon
//   LISTA_ID                -> , id LISTA_ID | epsilon
//   COMANDOS                -> COMANDO COMANDOS | epsilon
//   COMANDO                 -> LECTURA | ESCRITURA | ASIGNACION | FUNCION
//                            | SELECCION | MIENTRAS | RETORNO
//   LECTURA                 -> scanf ( TERMINO_LECTURA NUEVO_TERMINO_LECTURA ) ;
//   TERMINO_LECTURA         -> id DIMENSION2
//   NUEVO_TERMINO_LECTURA   -> , TERMINO_LECTURA NUEVO_TERMINO_LECTURA | epsilon
//   DIMENSION2              -> [ EXPRESION_ADITIVA ] DIMENSION2 | epsilon
//   ESCRITURA               -> println ( TERMINO_ESCRITURA NUEVO_TERMINO_ESCRITURA ) ;
//   TERMINO_ESCRITURA       -> id DIMENSION2 | CONSTANTE | texto
//   NUEVO_TERMINO_ESCRITURA -> , TERMINO_ESCRITURA NUEVO_TERMINO_ESCRITURA | epsilon
//   SELECCION               -> if ( EXPRESION ) BLOQUE SINO
//   SINO                    -> else BLOQUE | epsilon
//   MIENTRAS                -> while ( EXPRESION ) BLOQUE
//   ASIGNACION              -> id = COMPLEMENTO ;
//   COMPLEMENTO             -> EXPRESION | FUNCION
//   FUNCION                 -> func id ( ARGUMENTOS )
//   ARGUMENTOS              -> EXPRESION NUEVO_ARGUMENTO | epsilon
//   NUEVO_ARGUMENTO         -> , EXPRESION NUEVO_ARGUMENTO | epsilon
//   RETORNO                 -> return EXPRESION ;
//   EXPRESION               -> EXPRESION_O
//   EXPRESION_O             -> EXPRESION_Y EXPRESION_O2
//   EXPRESION_O2            -> || EXPRESION_Y EXPRESION_O2 | epsilon
//   EXPRESION_Y             -> EXPRESION_RELACIONAL EXPRESION_Y2
//   EXPRESION_Y2            -> && EXPRESION_RELACIONAL EXPRESION_Y2 | epsilon
//   EXPRESION_RELACIONAL    -> EXPRESION_ADITIVA EXPRESION_RELACIONAL2
//   EXPRESION_RELACIONAL2   -> comp EXPRESION_ADITIVA | epsilon
//   EXPRESION_ADITIVA       -> EXPRESION_MULTIPLICATIVA EXPRESION_ADITIVA2
//   EXPRESION_ADITIVA2      -> OPERADOR_ADITIVO EXPRESION_MULTIPLICATIVA
//                              EXPRESION_ADITIVA2 | epsilon
//   OPERADOR_ADITIVO        -> + | -
//   EXPRESION_MULTIPLICATIVA-> FACTOR EXPRESION_MULTIPLICATIVA2
//   EXPRESION_MULTIPLICATIVA2 -> OPERADOR_MULTIPLICATIVO FACTOR
//                              EXPRESION_MULTIPLICATIVA2 | epsilon
//   OPERADOR_MULTIPLICATIVO -> * | / | %
//   FACTOR                  -> SIGNO TERMINO | texto | ! FACTOR | ( EXPRESION )
//   TERMINO                 -> id DIMENSION2 | CONSTANTE
//   SIGNO                   -> + | - | epsilon
//   CONSTANTE               -> num_int | num_dec
//
// Las funciones del parser llevan el nombre del no terminal que reconocen.
class Parser {
public:
    explicit Parser(const std::string& rutaArchivo);

    // Ejecuta el analisis sintactico. Devuelve true solo si no hubo errores.
    bool analizar();

    bool archivoAbierto() const { return abierto; }
    const std::vector<Token>& obtenerTokens() const { return tokens; }
    const std::vector<ErrorCompilacion>& obtenerErrores() const { return errores; }
    const std::vector<std::string>& obtenerTraza() const { return traza; }
    const std::vector<std::string>& obtenerLineas() const { return lineas; }
    const TablaSimbolos& obtenerTablaSimbolos() const { return tabla; }

    int totalFunciones() const { return funciones; }
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
    std::vector<std::string> traza;
    TablaSimbolos tabla;

    bool abierto = false;
    int nivel = 0;
    int funciones = 0;
    int declaraciones = 0;
    int asignaciones = 0;

    // --- Utilidades del flujo --------------------------------------------
    const Token& actual() const;
    const Token& siguiente() const;
    bool esFin() const;
    bool siguienteEs(TokenType tipo) const;
    void avanzar();
    bool comprobar(TokenType tipo) const;
    bool consumir(TokenType tipo);
    static bool esTipoBase(TokenType tipo);
    static std::string describirToken(const Token& token);
    std::string textoEntre(std::size_t inicio, std::size_t fin) const;
    void sincronizar();
    std::size_t saltarTipoEn(std::size_t indice) const;
    bool esInicioFuncion() const;

    // --- Errores ----------------------------------------------------------
    void reportar(TipoError tipo, int linea, int columna,
                  const std::string& mensaje, const std::string& pista = "");
    void errorActual(const std::string& esperado, const std::string& contexto,
                     const std::string& pista = "");
    void errorPuntoComa(const std::string& contexto);

    // --- Traza ------------------------------------------------------------
    void abrirNodo(const std::string& etiqueta);
    void cerrarNodo();
    void nodo(const std::string& etiqueta);

    // --- Producciones de la gramatica ------------------------------------
    bool reconocerPrograma();
    bool reconocerListaFunciones();
    bool reconocerDeclaracionFuncion();
    InfoTipo reconocerTipoRetorno();
    InfoTipo reconocerTipo();
    InfoTipo reconocerTipoBase();
    bool reconocerDimension(InfoTipo& tipo);
    bool reconocerParametros();
    bool reconocerPrincipal();
    bool reconocerBloque();
    bool reconocerListaVariables();
    bool reconocerDeclaracion();
    bool reconocerListaId(const InfoTipo& tipo);
    bool reconocerComandos();
    bool reconocerComando();
    bool reconocerLectura();
    bool reconocerTerminoLectura();
    bool reconocerDimension2();
    bool reconocerEscritura();
    bool reconocerTerminoEscritura();
    bool reconocerSeleccion();
    bool reconocerSino();
    bool reconocerMientras();
    bool reconocerAsignacion();
    bool reconocerComplemento();
    bool reconocerFuncion();
    bool reconocerArgumentos();
    bool reconocerRetorno();
    bool reconocerExpresion();
    bool reconocerExpresionO();
    bool reconocerExpresionY();
    bool reconocerExpresionRelacional();
    bool reconocerExpresionAditiva();
    bool reconocerExpresionMultiplicativa();
    bool reconocerFactor();
    bool reconocerTermino();
    bool reconocerConstante();
};

#endif  // PARSER_H
