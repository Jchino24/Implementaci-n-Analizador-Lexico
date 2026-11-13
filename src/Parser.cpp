#include "Parser.h"

#include <fstream>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Construccion: lectura del archivo e integracion con el lexer
// ---------------------------------------------------------------------------

Parser::Parser(const std::string& rutaArchivo) {
    std::ifstream entrada(rutaArchivo);
    if (!entrada.is_open()) {
        return;
    }

    abierto = true;
    std::string linea;
    while (std::getline(entrada, linea)) {
        if (!linea.empty() && linea.back() == '\r') {
            linea.pop_back();
        }
        lineas.push_back(linea);
    }
    entrada.close();

    // Integracion con el lexer: se obtiene toda la secuencia de tokens.
    Lexer lexer(rutaArchivo);
    Token token;
    do {
        token = lexer.getNextToken();
        tokens.push_back(token);
        if (token.tipo != DESCONOCIDO) {
            flujo.push_back(token);
        }
    } while (token.tipo != FIN_ARCHIVO);

    if (flujo.empty() || flujo.back().tipo != FIN_ARCHIVO) {
        Token fin;
        fin.tipo = FIN_ARCHIVO;
        fin.linea = static_cast<int>(lineas.size());
        fin.columna = 1;
        flujo.push_back(fin);
    }

    for (const ErrorLexico& error : lexer.obtenerErrores()) {
        reportar(TipoError::LEXICO, error.linea, error.columna, error.mensaje);
    }
}

bool Parser::analizar() {
    if (!abierto) {
        reportar(TipoError::SINTACTICO, 1, 1,
                 "no se pudo abrir el archivo .lp para analizarlo");
        return false;
    }

    pos = 0;
    return reconocerPrograma() && errores.empty();
}

// ---------------------------------------------------------------------------
// Utilidades del flujo de tokens
// ---------------------------------------------------------------------------

const Token& Parser::actual() const {
    return flujo[pos];
}

const Token& Parser::siguiente() const {
    if (pos + 1 < flujo.size()) {
        return flujo[pos + 1];
    }
    return flujo[pos];
}

bool Parser::esFin() const {
    return actual().tipo == FIN_ARCHIVO;
}

bool Parser::siguienteEs(TokenType tipo) const {
    return siguiente().tipo == tipo;
}

void Parser::avanzar() {
    if (pos + 1 < flujo.size()) {
        ++pos;
    }
}

bool Parser::comprobar(TokenType tipo) const {
    return actual().tipo == tipo;
}

bool Parser::consumir(TokenType tipo) {
    if (comprobar(tipo)) {
        avanzar();
        return true;
    }
    return false;
}

bool Parser::esTipoBase(TokenType tipo) {
    return tipo == INT || tipo == FLOAT || tipo == CHAR || tipo == BOOLEAN;
}

std::string Parser::describirToken(const Token& token) {
    if (token.tipo == FIN_ARCHIVO) {
        return "fin de archivo";
    }
    if (token.lexema.empty()) {
        return tokenTypeToString(token.tipo);
    }
    const std::string tipo = tokenTypeToString(token.tipo);
    if (tipo == token.lexema) {
        return "'" + token.lexema + "'";
    }
    return "'" + token.lexema + "' (" + tipo + ")";
}

std::size_t Parser::saltarTipoEn(std::size_t indice) const {
    if (indice >= flujo.size()) {
        return indice;
    }
    if (flujo[indice].tipo == VOID) {
        return indice + 1;
    }
    if (!esTipoBase(flujo[indice].tipo)) {
        return indice;
    }

    ++indice;
    while (indice < flujo.size() && flujo[indice].tipo == CORCH_IZQ) {
        ++indice;
        if (indice < flujo.size() && flujo[indice].tipo == NUM_INT) {
            ++indice;
        }
        if (indice < flujo.size() && flujo[indice].tipo == CORCH_DER) {
            ++indice;
        } else {
            break;
        }
    }
    return indice;
}

bool Parser::esInicioFuncion() const {
    const std::size_t despuesDelTipo = saltarTipoEn(pos);
    return despuesDelTipo < flujo.size() && flujo[despuesDelTipo].tipo == ID;
}

// ---------------------------------------------------------------------------
// Reporte de errores
// ---------------------------------------------------------------------------

void Parser::reportar(TipoError tipo, int linea, int columna,
                      const std::string& mensaje, const std::string& pista) {
    ErrorCompilacion error;
    error.tipo = tipo;
    error.linea = linea;
    error.columna = columna;
    error.mensaje = mensaje;
    error.pista = pista;
    errores.push_back(std::move(error));
}

void Parser::errorActual(const std::string& esperado, const std::string& contexto,
                         const std::string& pista) {
    const Token& token = actual();
    std::string mensaje = "se esperaba " + esperado;
    if (!contexto.empty()) {
        mensaje += " en " + contexto;
    }
    mensaje += ", pero se encontró " + describirToken(token);
    reportar(TipoError::SINTACTICO, token.linea, token.columna, mensaje, pista);
}

void Parser::errorPuntoComa(const std::string& contexto) {
    const Token& token = actual();
    std::string pista;
    if (pos > 0) {
        const Token& anterior = flujo[pos - 1];
        pista = "Falta ';' al final de la sentencia de la línea " +
                std::to_string(anterior.linea) + ".";
    } else {
        pista = "Agregue ';' al final de la sentencia.";
    }
    reportar(TipoError::SINTACTICO, token.linea, token.columna,
             "se esperaba ';' al final de " + contexto + ", pero se encontró " +
                 describirToken(token),
             pista);
}

// ---------------------------------------------------------------------------
// PROGRAMA -> LISTA_FUNCIONES PRINCIPAL
// ---------------------------------------------------------------------------

bool Parser::reconocerPrograma() {
    if (!reconocerListaFunciones()) {
        return false;
    }

    // La gramática base usa main(); se acepta void main() por compatibilidad
    // con el ejemplo del curso.
    const bool hayPrincipal =
        comprobar(MAIN) || (comprobar(VOID) && siguienteEs(MAIN));
    if (!hayPrincipal) {
        errorActual("'main' (PRINCIPAL)", "PROGRAMA",
                    "El programa debe terminar con main() y su bloque.");
        return false;
    }

    if (!reconocerPrincipal()) {
        return false;
    }

    if (!esFin()) {
        errorActual("fin de archivo después del bloque principal", "PROGRAMA");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// LISTA_FUNCIONES -> DECLARACION_FUNCION LISTA_FUNCIONES | epsilon
// ---------------------------------------------------------------------------

bool Parser::reconocerListaFunciones() {
    while (esInicioFuncion()) {
        if (!reconocerDeclaracionFuncion()) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// DECLARACION_FUNCION -> TIPO_RETORNO id ( PARAMETROS ) BLOQUE
// ---------------------------------------------------------------------------

bool Parser::reconocerDeclaracionFuncion() {
    const InfoTipo retorno = reconocerTipoRetorno();
    if (!retorno.valido) {
        return false;
    }

    if (!comprobar(ID)) {
        errorActual("un nombre de función (ID)", "DECLARACION_FUNCION",
                    "Ejemplo: int suma(int a, int b) { ... }");
        return false;
    }
    avanzar();

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después del nombre de la función", "DECLARACION_FUNCION",
                    "Formato esperado: tipo nombre(parámetros) { ... }");
        return false;
    }

    if (!comprobar(PAR_DER)) {
        if (!reconocerParametros()) {
            return false;
        }
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar los parámetros de la función",
                    "DECLARACION_FUNCION");
        return false;
    }

    return reconocerBloque();
}

// ---------------------------------------------------------------------------
// TIPO_RETORNO -> TIPO | void
// ---------------------------------------------------------------------------

Parser::InfoTipo Parser::reconocerTipoRetorno() {
    if (comprobar(VOID)) {
        InfoTipo tipo;
        tipo.completo = "void";
        tipo.valido = true;
        avanzar();
        return tipo;
    }
    return reconocerTipo();
}

// ---------------------------------------------------------------------------
// TIPO -> TIPO_BASE DIMENSION
// ---------------------------------------------------------------------------

Parser::InfoTipo Parser::reconocerTipo() {
    InfoTipo tipo = reconocerTipoBase();
    if (!tipo.valido) {
        return tipo;
    }
    if (!reconocerDimension(tipo)) {
        tipo.valido = false;
    }
    return tipo;
}

// TIPO_BASE -> char | float | int | boolean
Parser::InfoTipo Parser::reconocerTipoBase() {
    InfoTipo tipo;
    const Token& token = actual();

    if (esTipoBase(token.tipo)) {
        tipo.completo = token.lexema;
        tipo.valido = true;
        avanzar();
    } else {
        errorActual("un tipo base ('char', 'float', 'int', 'boolean')",
                    "TIPO_BASE");
    }
    return tipo;
}

// DIMENSION -> [ num_int ] DIMENSION | epsilon
bool Parser::reconocerDimension(InfoTipo& tipo) {
    while (consumir(CORCH_IZQ)) {
        if (!comprobar(NUM_INT)) {
            errorActual("un tamaño entero (NUM_INT)", "DIMENSION",
                        "Ejemplo: int[10] notas;");
            return false;
        }

        const std::string tamano = actual().lexema;
        avanzar();

        if (!consumir(CORCH_DER)) {
            errorActual("']' para cerrar la DIMENSION", "DIMENSION");
            return false;
        }

        tipo.completo += "[" + tamano + "]";
    }
    return true;
}

// ---------------------------------------------------------------------------
// PARAMETROS -> TIPO id LISTA_PARAMETROS | epsilon
// ---------------------------------------------------------------------------

bool Parser::reconocerParametros() {
    while (true) {
        const InfoTipo tipo = reconocerTipo();
        if (!tipo.valido) {
            return false;
        }

        if (!comprobar(ID)) {
            errorActual("un nombre de parámetro (ID)", "PARAMETROS");
            return false;
        }
        avanzar();

        if (comprobar(CORCH_IZQ)) {
            errorActual("dimensiones antes del nombre del parámetro", "PARAMETROS",
                        "Ejemplo: int[10] a");
            return false;
        }

        if (!consumir(COMA)) {
            return true;
        }
    }
}

// ---------------------------------------------------------------------------
// PRINCIPAL -> [void] main ( ) BLOQUE
// ---------------------------------------------------------------------------

bool Parser::reconocerPrincipal() {
    if (comprobar(VOID) && siguienteEs(MAIN)) {
        avanzar();
    }

    if (!consumir(MAIN)) {
        errorActual("'main'", "PRINCIPAL");
        return false;
    }

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después de 'main'", "PRINCIPAL",
                    "La firma debe ser: main() { ... }");
        return false;
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar 'main()'", "PRINCIPAL",
                    "La firma debe ser: main() { ... }");
        return false;
    }

    return reconocerBloque();
}

// ---------------------------------------------------------------------------
// BLOQUE -> { LISTA_VARIABLES COMANDOS }
// ---------------------------------------------------------------------------

bool Parser::reconocerBloque() {
    if (!consumir(LLAVE_IZQ)) {
        errorActual("'{' para iniciar el bloque", "BLOQUE",
                    "El cuerpo debe ir entre llaves: { ... }");
        return false;
    }

    const int lineaApertura = flujo[pos - 1].linea;

    if (!reconocerListaVariables()) {
        return false;
    }
    if (!reconocerComandos()) {
        return false;
    }

    if (!consumir(LLAVE_DER)) {
        const Token& token = actual();
        int lineaError = token.linea;
        int columnaError = token.columna;
        if (lineaError > static_cast<int>(lineas.size()) && !lineas.empty()) {
            lineaError = static_cast<int>(lineas.size());
            columnaError = static_cast<int>(lineas.back().size()) + 1;
        }
        reportar(TipoError::SINTACTICO, lineaError, columnaError,
                 "bloque sin cerrar: se esperaba '}' para cerrar el BLOQUE",
                 "La llave '{' de la línea " + std::to_string(lineaApertura) +
                     " no tiene su '}' correspondiente.");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// LISTA_VARIABLES -> TIPO id LISTA_ID ; LISTA_VARIABLES | epsilon
// ---------------------------------------------------------------------------

bool Parser::reconocerListaVariables() {
    while (esTipoBase(actual().tipo)) {
        if (!reconocerDeclaracion()) {
            return false;
        }
    }
    return true;
}

// DECLARACION -> TIPO id LISTA_ID ;
bool Parser::reconocerDeclaracion() {
    const InfoTipo tipo = reconocerTipo();
    if (!tipo.valido) {
        return false;
    }

    if (!reconocerListaId()) {
        return false;
    }

    if (!consumir(PUNTO_COMA)) {
        errorPuntoComa("la declaración (LISTA_VARIABLES)");
        return false;
    }
    return true;
}

// LISTA_ID -> , id LISTA_ID | epsilon
bool Parser::reconocerListaId() {
    if (!comprobar(ID)) {
        errorActual("un nombre de variable (ID)", "LISTA_ID",
                    "Ejemplo: int edad, promedio;");
        return false;
    }
    avanzar();

    if (comprobar(CORCH_IZQ)) {
        errorActual("el identificador sin dimensión", "LISTA_ID",
                    "Las dimensiones van antes del nombre: int[10] notas;");
        return false;
    }

    while (consumir(COMA)) {
        if (!comprobar(ID)) {
            errorActual("un nombre de variable (ID) después de ','", "LISTA_ID");
            return false;
        }
        avanzar();
    }
    return true;
}

// ---------------------------------------------------------------------------
// COMANDOS -> ASIGNACION COMANDOS | epsilon
// ---------------------------------------------------------------------------

bool Parser::reconocerComandos() {
    while (!esFin() && !comprobar(LLAVE_DER)) {
        if (!reconocerComando()) {
            return false;
        }
    }
    return true;
}

// COMANDO -> ASIGNACION
bool Parser::reconocerComando() {
    if (comprobar(ID)) {
        return reconocerAsignacion();
    }

    errorActual("una asignación", "COMANDO",
                "En esta etapa el bloque solo admite asignaciones.");
    return false;
}

// ---------------------------------------------------------------------------
// ASIGNACION -> id = COMPLEMENTO ;
// ---------------------------------------------------------------------------

bool Parser::reconocerAsignacion() {
    if (!comprobar(ID)) {
        errorActual("un identificador al inicio de la asignación", "ASIGNACION");
        return false;
    }
    avanzar();

    if (!consumir(ASIGNACION)) {
        errorActual("'=' en la asignación", "ASIGNACION",
                    "Formato esperado: variable = expresión;");
        return false;
    }

    if (!reconocerComplemento()) {
        return false;
    }

    if (!consumir(PUNTO_COMA)) {
        errorPuntoComa("la asignación (ASIGNACION)");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// COMPLEMENTO -> EXPRESION
// ---------------------------------------------------------------------------

bool Parser::reconocerComplemento() {
    return reconocerExpresion();
}

// ---------------------------------------------------------------------------
// EXPRESION -> EXPRESION_O
// ---------------------------------------------------------------------------

bool Parser::reconocerExpresion() {
    return reconocerExpresionO();
}

// EXPRESION_O -> EXPRESION_Y ( || EXPRESION_Y )*
bool Parser::reconocerExpresionO() {
    if (!reconocerExpresionY()) {
        return false;
    }
    while (comprobar(OR)) {
        avanzar();
        if (!reconocerExpresionY()) {
            return false;
        }
    }
    return true;
}

// EXPRESION_Y -> EXPRESION_RELACIONAL ( && EXPRESION_RELACIONAL )*
bool Parser::reconocerExpresionY() {
    if (!reconocerExpresionRelacional()) {
        return false;
    }
    while (comprobar(AND)) {
        avanzar();
        if (!reconocerExpresionRelacional()) {
            return false;
        }
    }
    return true;
}

// EXPRESION_RELACIONAL -> EXPRESION_ADITIVA [ comp EXPRESION_ADITIVA ]
bool Parser::reconocerExpresionRelacional() {
    if (!reconocerExpresionAditiva()) {
        return false;
    }
    if (consumir(COMP)) {
        if (!reconocerExpresionAditiva()) {
            return false;
        }
    }
    return true;
}

// EXPRESION_ADITIVA -> EXPRESION_MULTIPLICATIVA ( (+|-) ... )*
bool Parser::reconocerExpresionAditiva() {
    if (!reconocerExpresionMultiplicativa()) {
        return false;
    }
    while (comprobar(SUMA) || comprobar(RESTA)) {
        avanzar();
        if (!reconocerExpresionMultiplicativa()) {
            return false;
        }
    }
    return true;
}

// EXPRESION_MULTIPLICATIVA -> FACTOR ( (*|/|%) FACTOR )*
bool Parser::reconocerExpresionMultiplicativa() {
    if (!reconocerFactor()) {
        return false;
    }
    while (comprobar(MULT) || comprobar(DIV) || comprobar(MOD)) {
        avanzar();
        if (!reconocerFactor()) {
            return false;
        }
    }
    return true;
}

// FACTOR -> SIGNO TERMINO | texto | ! FACTOR | ( EXPRESION )
bool Parser::reconocerFactor() {
    if (comprobar(SUMA) || comprobar(RESTA)) {
        avanzar();
        return reconocerTermino();
    }

    if (consumir(TEXTO)) {
        return true;
    }

    if (consumir(NOT)) {
        return reconocerFactor();
    }

    if (consumir(PAR_IZQ)) {
        if (!reconocerExpresion()) {
            return false;
        }
        if (!consumir(PAR_DER)) {
            errorActual("')' para cerrar la expresión", "FACTOR");
            return false;
        }
        return true;
    }

    return reconocerTermino();
}

// TERMINO -> id DIMENSION2 | CONSTANTE
bool Parser::reconocerTermino() {
    if (comprobar(ID)) {
        avanzar();
        return reconocerDimension2();
    }
    return reconocerConstante();
}

// CONSTANTE -> num_int | num_dec
bool Parser::reconocerConstante() {
    if (consumir(NUM_INT) || consumir(NUM_DEC)) {
        return true;
    }
    errorActual("una constante numérica (NUM_INT o NUM_DEC)", "CONSTANTE",
                "Ejemplo: 20, 15.5");
    return false;
}

// DIMENSION2 -> [ EXPRESION_ADITIVA ] DIMENSION2 | epsilon
bool Parser::reconocerDimension2() {
    while (consumir(CORCH_IZQ)) {
        if (!reconocerExpresionAditiva()) {
            return false;
        }
        if (!consumir(CORCH_DER)) {
            errorActual("']' para cerrar el subíndice", "DIMENSION2");
            return false;
        }
    }
    return true;
}
