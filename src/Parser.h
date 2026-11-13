#ifndef PARSER_H
#define PARSER_H

#include <cstddef>
#include <string>
#include <vector>

#include "Lexer.h"
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

// Analizador sintactico recursivo descendente para el Entregable 1 de LP.
//
// Alcance: estructura general del programa, funciones antes de main,
// bloques, declaraciones y asignaciones con expresiones.
//
//   PROGRAMA             -> LISTA_FUNCIONES PRINCIPAL
//   LISTA_FUNCIONES      -> DECLARACION_FUNCION LISTA_FUNCIONES | epsilon
//   DECLARACION_FUNCION  -> TIPO_RETORNO id ( PARAMETROS ) BLOQUE
//   TIPO_RETORNO         -> TIPO | void
//   TIPO                 -> TIPO_BASE DIMENSION
//   TIPO_BASE            -> char | float | int | boolean
//   DIMENSION            -> [ num_int ] DIMENSION | epsilon
//   PARAMETROS           -> TIPO id LISTA_PARAMETROS | epsilon
//   LISTA_PARAMETROS     -> , TIPO id LISTA_PARAMETROS | epsilon
//   PRINCIPAL            -> [void] main ( ) BLOQUE
//   BLOQUE               -> { LISTA_VARIABLES COMANDOS }
//   LISTA_VARIABLES      -> TIPO id LISTA_ID ; LISTA_VARIABLES | epsilon
//   LISTA_ID             -> , id LISTA_ID | epsilon
//   COMANDOS             -> ASIGNACION COMANDOS | epsilon
//   ASIGNACION           -> id = COMPLEMENTO ;
//   COMPLEMENTO          -> EXPRESION
//   EXPRESION            -> EXPRESION_O
//   EXPRESION_O          -> EXPRESION_Y ( || EXPRESION_Y )*
//   EXPRESION_Y          -> EXPRESION_RELACIONAL ( && EXPRESION_RELACIONAL )*
//   EXPRESION_RELACIONAL -> EXPRESION_ADITIVA [ comp EXPRESION_ADITIVA ]
//   EXPRESION_ADITIVA    -> EXPRESION_MULTIPLICATIVA ( (+|-) ... )*
//   EXPRESION_MULTIPLICATIVA -> FACTOR ( (*|/|%) FACTOR )*
//   FACTOR               -> SIGNO TERMINO | texto | ! FACTOR | ( EXPRESION )
//   TERMINO              -> id DIMENSION2 | CONSTANTE
//   DIMENSION2           -> [ EXPRESION_ADITIVA ] DIMENSION2 | epsilon
//   SIGNO                -> + | - | epsilon
//   CONSTANTE            -> num_int | num_dec
//
// `void main()` se acepta por compatibilidad con el ejemplo del curso.
class Parser {
public:
    explicit Parser(const std::string& rutaArchivo);

    // Ejecuta el analisis sintactico. Devuelve true solo si no hubo errores.
    bool analizar();

    bool archivoAbierto() const { return abierto; }
    const std::vector<Token>& obtenerTokens() const { return tokens; }
    const std::vector<ErrorCompilacion>& obtenerErrores() const { return errores; }
    const std::vector<std::string>& obtenerLineas() const { return lineas; }

private:
    // Informacion que produce TIPO.
    struct InfoTipo {
        std::string completo;  // int, float[10], int[3][3], ...
        bool valido = false;
    };

    // --- Flujo de tokens -------------------------------------------------
    std::vector<Token> tokens;  // Todos los tokens, para el reporte lexico.
    std::vector<Token> flujo;   // Tokens utiles (sin DESCONOCIDO) + FIN_ARCHIVO.
    std::size_t pos = 0;

    std::vector<std::string> lineas;
    std::vector<ErrorCompilacion> errores;

    bool abierto = false;

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
    std::size_t saltarTipoEn(std::size_t indice) const;
    bool esInicioFuncion() const;

    // --- Errores ----------------------------------------------------------
    void reportar(TipoError tipo, int linea, int columna,
                  const std::string& mensaje, const std::string& pista = "");
    void errorActual(const std::string& esperado, const std::string& contexto,
                     const std::string& pista = "");
    void errorPuntoComa(const std::string& contexto);

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
    bool reconocerListaId();
    bool reconocerComandos();
    bool reconocerComando();
    bool reconocerAsignacion();
    bool reconocerComplemento();
    bool reconocerExpresion();
    bool reconocerExpresionO();
    bool reconocerExpresionY();
    bool reconocerExpresionRelacional();
    bool reconocerExpresionAditiva();
    bool reconocerExpresionMultiplicativa();
    bool reconocerFactor();
    bool reconocerTermino();
    bool reconocerConstante();
    bool reconocerDimension2();
};

#endif  // PARSER_H
