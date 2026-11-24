#ifndef PARSER_H
#define PARSER_H

#include <cstddef>
#include <string>
#include <vector>

#include "Ast.h"
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

// Analizador sintactico recursivo descendente para el lenguaje LP.
//
// Gramatica implementada:
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
//   PRINCIPAL               -> [void] main ( ) BLOQUE
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
// `void main()` se acepta por compatibilidad con el ejemplo del curso.
class Parser {
public:
    explicit Parser(const std::string& rutaArchivo);

    // Ejecuta el analisis sintactico y construye el AST.
    bool analizar();

    bool archivoAbierto() const { return abierto; }
    const std::vector<Token>& obtenerTokens() const { return tokens; }
    const std::vector<ErrorCompilacion>& obtenerErrores() const { return errores; }
    const std::vector<std::string>& obtenerLineas() const { return lineas; }
    const AstNodo& obtenerAst() const { return ast; }

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
    AstNodo ast;

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
    void sincronizar();

    // --- Errores ----------------------------------------------------------
    void reportar(TipoError tipo, int linea, int columna,
                  const std::string& mensaje, const std::string& pista = "");
    void errorActual(const std::string& esperado, const std::string& contexto,
                     const std::string& pista = "");
    void errorPuntoComa(const std::string& contexto);

    // --- Producciones de la gramatica ------------------------------------
    bool reconocerPrograma(AstNodo& salida);
    bool reconocerListaFunciones(std::vector<AstNodo>& funciones);
    bool reconocerDeclaracionFuncion(AstNodo& funcion);
    bool reconocerTipoRetorno(InfoTipo& tipo);
    bool reconocerTipo(InfoTipo& tipo);
    bool reconocerTipoBase(InfoTipo& tipo);
    bool reconocerDimension(InfoTipo& tipo);
    bool reconocerParametros(std::vector<AstNodo>& parametros);
    bool reconocerPrincipal(AstNodo& principal);
    bool reconocerBloque(AstNodo& bloque);
    bool reconocerListaVariables(std::vector<AstNodo>& declaraciones);
    bool reconocerDeclaracion(AstNodo& declaracion);
    bool reconocerListaId(AstNodo& declaracion);
    bool reconocerComandos(std::vector<AstNodo>& comandos);
    bool reconocerComando(AstNodo& comando);
    bool reconocerLectura(AstNodo& comando);
    bool reconocerEscritura(AstNodo& comando);
    bool reconocerSeleccion(AstNodo& comando);
    bool reconocerSino(AstNodo& sino);
    bool reconocerMientras(AstNodo& comando);
    bool reconocerAsignacion(AstNodo& comando);
    bool reconocerComplemento(AstNodo& expr);
    bool reconocerFuncion(AstNodo& llamada);
    bool reconocerArgumentos(std::vector<AstNodo>& argumentos);
    bool reconocerRetorno(AstNodo& comando);
    bool reconocerExpresion(AstNodo& expr);
    bool reconocerExpresionO(AstNodo& expr);
    bool reconocerExpresionY(AstNodo& expr);
    bool reconocerExpresionRelacional(AstNodo& expr);
    bool reconocerExpresionAditiva(AstNodo& expr);
    bool reconocerExpresionMultiplicativa(AstNodo& expr);
    bool reconocerFactor(AstNodo& expr);
    bool reconocerTermino(AstNodo& expr);
    bool reconocerConstante(AstNodo& expr);
    bool reconocerDimension2(AstNodo& termino);
};

#endif  // PARSER_H
