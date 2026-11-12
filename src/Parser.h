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
// Gramatica implementada (EBNF):
//
//   PROGRAMA        -> LISTA_FUNCIONES PRINCIPAL
//   LISTA_FUNCIONES -> FUNCION*
//   FUNCION         -> TIPO_RETORNO ID ( LISTA_PARAMETROS ) BLOQUE
//   TIPO_RETORNO    -> void | TIPO_BASE
//   PRINCIPAL       -> [TIPO_RETORNO] main ( ) BLOQUE
//   BLOQUE          -> { LISTA_VARIABLES COMANDOS }
//   LISTA_VARIABLES -> (TIPO LISTA_ID ;)*
//   COMANDOS        -> COMANDO*
//   COMANDO         -> ASIGNACION | IF | WHILE | LLAMADA
//                    | RETURN | SCANF | PRINTLN
//   TIPO            -> TIPO_BASE DIMENSION*
//   TIPO_BASE       -> int | float | char | boolean
//   DIMENSION       -> [ NUM_INT ]
//   LISTA_ID        -> ID DIMENSION* ( , ID DIMENSION* )*
//   ASIGNACION      -> ID ( [ COMPLEMENTO ] )* = COMPLEMENTO ;
//   IF              -> if ( COMPLEMENTO ) BLOQUE [ else ( IF | BLOQUE ) ]
//   WHILE           -> while ( COMPLEMENTO ) BLOQUE
//   LLAMADA         -> func ID ( LISTA_ARGUMENTOS ) [ ; ]
//   RETURN          -> return [ COMPLEMENTO ] ;
//   SCANF/PRINTLN   -> (scanf|println) ( LISTA_ARGUMENTOS ) ;
//   LISTA_PARAMETROS-> ( TIPO ID (DIMENSION*) ( , ... )* )?
//   LISTA_ARGUMENTOS-> ( COMPLEMENTO ( , COMPLEMENTO )* )?
//   COMPLEMENTO     -> OR
//   OR              -> AND ( || AND )*
//   AND             -> REL ( && REL )*
//   REL             -> ADD ( COMP ADD )*
//   ADD             -> MUL ( (+|-) MUL )*
//   MUL             -> UNARIO ( (*|/|%) UNARIO )*
//   UNARIO          -> (-|!) UNARIO | PRIMARIO
//   PRIMARIO        -> NUM_INT | NUM_DEC | TEXTO
//                    | func ID ( LISTA_ARGUMENTOS )
//                    | ID ( [ COMPLEMENTO ] )*
//                    | ( COMPLEMENTO )
//
// La llamada a funcion usa el terminal `func`, incorporado al lexer.
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
    bool reconocerFuncion();
    bool reconocerPrincipal();
    bool reconocerBloque();
    bool reconocerListaVariables();
    bool reconocerDeclaracion();
    bool reconocerListaVariablesDeclaracion(const InfoTipo& tipo);
    bool reconocerComandos();
    bool reconocerComando();
    bool reconocerAsignacion();
    bool reconocerIf();
    bool reconocerWhile();
    bool reconocerLlamada(bool consumirPuntoComa);
    bool reconocerLlamadaIncorporada();
    bool reconocerReturn();
    bool reconocerListaParametros();
    bool reconocerListaArgumentos();
    InfoTipo reconocerTipoRetorno();
    InfoTipo reconocerTipo();
    InfoTipo reconocerTipoBase(bool permitirVoid);
    bool reconocerDimension(InfoTipo& tipo);
    std::vector<std::string> reconocerListaId(const InfoTipo& tipo, bool& ok);
    bool reconocerSubindices();
    bool reconocerComplemento();
    bool reconocerOr();
    bool reconocerAnd();
    bool reconocerRelacional();
    bool reconocerAditivo();
    bool reconocerMultiplicativo();
    bool reconocerUnario();
    bool reconocerPrimario();
};

#endif  // PARSER_H
