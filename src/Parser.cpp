#include "Parser.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// Construccion: lectura del archivo, integracion con el lexer
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

    // Integracion con el analizador lexico: se obtiene toda la secuencia.
    Lexer lexer(rutaArchivo);
    Token token;
    do {
        token = lexer.getNextToken();
        tokens.push_back(token);
        if (token.tipo != DESCONOCIDO) {
            flujo.push_back(token);
        }
    } while (token.tipo != FIN_ARCHIVO);

    // Garantia: el flujo siempre termina en FIN_ARCHIVO.
    if (flujo.empty() || flujo.back().tipo != FIN_ARCHIVO) {
        Token fin;
        fin.tipo = FIN_ARCHIVO;
        fin.linea = static_cast<int>(lineas.size());
        fin.columna = 1;
        flujo.push_back(fin);
    }

    // Los caracteres no reconocidos ya fueron detectados por el lexer.
    for (const ErrorLexico& error : lexer.obtenerErrores()) {
        reportar(TipoError::LEXICO, error.linea, error.columna, error.mensaje);
    }
}

Parser::CierreNodo::~CierreNodo() {
    parser->cerrarNodo();
}

bool Parser::analizar() {
    if (!abierto) {
        reportar(TipoError::SINTACTICO, 1, 1,
                 "no se pudo abrir el archivo .lp para analizarlo");
        return false;
    }

    pos = 0;
    const bool programaValido = reconocerPrograma();

    // Ordenar por posición para que el diagnóstico se lea de arriba hacia abajo.
    const auto porPosicion = [](const ErrorCompilacion& a,
                                const ErrorCompilacion& b) {
        if (a.linea != b.linea) {
            return a.linea < b.linea;
        }
        return a.columna < b.columna;
    };
    std::stable_sort(errores.begin(), errores.end(), porPosicion);

    return programaValido && errores.empty();
}

int Parser::totalErroresLexicos() const {
    int total = 0;
    for (const ErrorCompilacion& error : errores) {
        if (error.tipo == TipoError::LEXICO) {
            ++total;
        }
    }
    return total;
}

int Parser::totalErroresSintacticos() const {
    int total = 0;
    for (const ErrorCompilacion& error : errores) {
        if (error.tipo == TipoError::SINTACTICO) {
            ++total;
        }
    }
    return total;
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

std::string Parser::textoEntre(std::size_t inicio, std::size_t fin) const {
    std::string texto;
    for (std::size_t i = inicio; i < fin && i < flujo.size(); ++i) {
        if (flujo[i].tipo == FIN_ARCHIVO) {
            break;
        }
        if (!texto.empty()) {
            texto += " ";
        }
        texto += flujo[i].lexema;
    }
    return texto;
}

void Parser::sincronizar() {
    while (!esFin() && !comprobar(PUNTO_COMA) && !comprobar(LLAVE_DER)) {
        avanzar();
    }
}

bool Parser::esInicioFuncion() const {
    if (!(comprobar(VOID) || esTipoBase(actual().tipo))) {
        return false;
    }
    return siguienteEs(ID);
}

// ---------------------------------------------------------------------------
// Reporte de errores
// ---------------------------------------------------------------------------

void Parser::reportar(TipoError tipo, int linea, int columna,
                      const std::string& mensaje, const std::string& pista) {
    if (errores.size() >= 100) {
        return;
    }
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
// Traza de reconocimiento
// ---------------------------------------------------------------------------

void Parser::abrirNodo(const std::string& etiqueta) {
    traza.push_back(std::string(static_cast<std::size_t>(nivel) * 2, ' ') + etiqueta);
    ++nivel;
}

void Parser::cerrarNodo() {
    if (nivel > 0) {
        --nivel;
    }
}

void Parser::nodo(const std::string& etiqueta) {
    traza.push_back(std::string(static_cast<std::size_t>(nivel) * 2, ' ') + etiqueta);
}

// ---------------------------------------------------------------------------
// PROGRAMA -> LISTA_FUNCIONES PRINCIPAL
// ---------------------------------------------------------------------------

bool Parser::reconocerPrograma() {
    abrirNodo("PROGRAMA");
    CierreNodo cierre{this};

    bool ok = reconocerListaFunciones();
    const bool principalOk = reconocerPrincipal();

    if (!esFin()) {
        const Token& sobrante = actual();
        reportar(TipoError::SINTACTICO, sobrante.linea, sobrante.columna,
                 "contenido inesperado después del bloque principal: se encontró " +
                     describirToken(sobrante),
                 "Solo puede existir una función main() con su bloque al final.");
        ok = false;
        while (!esFin()) {
            avanzar();
        }
    }

    return ok && principalOk;
}

// ---------------------------------------------------------------------------
// LISTA_FUNCIONES -> FUNCION*
// ---------------------------------------------------------------------------

bool Parser::reconocerListaFunciones() {
    abrirNodo("LISTA_FUNCIONES");
    CierreNodo cierre{this};

    bool ok = true;
    while (esInicioFuncion()) {
        ok = reconocerFuncion() && ok;
    }
    return ok;
}

// ---------------------------------------------------------------------------
// FUNCION -> TIPO_RETORNO ID ( LISTA_PARAMETROS ) BLOQUE
// ---------------------------------------------------------------------------

bool Parser::reconocerFuncion() {
    abrirNodo("FUNCION");
    CierreNodo cierre{this};

    bool ok = true;
    const InfoTipo retorno = reconocerTipoRetorno();
    ok = retorno.valido && ok;

    if (!comprobar(ID)) {
        errorActual("un nombre de función (ID)", "FUNCION",
                    "Ejemplo: int suma(int a, int b) { ... }");
        return false;
    }

    const Token nombre = actual();
    nodo("ID: " + nombre.lexema);
    avanzar();
    ++funciones;

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después del nombre de la función", "FUNCION",
                    "Formato esperado: tipo nombre(parámetros) { ... }");
        ok = false;
        // Recuperacion: avanzar hasta el bloque de la funcion.
        while (!esFin() && !comprobar(LLAVE_IZQ)) {
            avanzar();
        }
    } else {
        if (!comprobar(PAR_DER)) {
            ok = reconocerListaParametros() && ok;
        }
        if (!consumir(PAR_DER)) {
            errorActual("')' para cerrar los parámetros de la función", "FUNCION");
            ok = false;
            while (!esFin() && !comprobar(PAR_DER) && !comprobar(LLAVE_IZQ)) {
                avanzar();
            }
            consumir(PAR_DER);
        }
    }

    if (comprobar(LLAVE_IZQ)) {
        ok = reconocerBloque() && ok;
    } else {
        errorActual("'{' para iniciar el bloque de la función", "FUNCION");
        ok = false;
    }

    return ok;
}

// ---------------------------------------------------------------------------
// PRINCIPAL -> [TIPO_RETORNO] main ( ) BLOQUE
// ---------------------------------------------------------------------------

bool Parser::reconocerPrincipal() {
    abrirNodo("PRINCIPAL");
    CierreNodo cierre{this};

    bool ok = true;

    // Se acepta tanto `main()` como `void main()` / `int main()`.
    if ((comprobar(VOID) || esTipoBase(actual().tipo)) && siguienteEs(MAIN)) {
        const InfoTipo retorno = reconocerTipoRetorno();
        ok = retorno.valido && ok;
    }

    if (!consumir(MAIN)) {
        errorActual("'main'", "PRINCIPAL",
                    "Todo programa LP debe terminar con main() y su bloque.");
        return false;
    }

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después de 'main'", "PRINCIPAL",
                    "La firma de main debe ser: main() { ... }");
        ok = false;
        consumir(PAR_DER);
    } else if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar los parámetros de main", "PRINCIPAL",
                    "En esta etapa main() no recibe parámetros.");
        ok = false;
        while (!esFin() && !comprobar(PAR_DER) && !comprobar(LLAVE_IZQ)) {
            avanzar();
        }
        consumir(PAR_DER);
    }

    nodo("main ( )");

    if (comprobar(LLAVE_IZQ)) {
        ok = reconocerBloque() && ok;
    } else {
        errorActual("'{' para iniciar el bloque de main", "PRINCIPAL",
                    "main() debe ir seguido de su bloque: { ... }");
        ok = false;
    }

    return ok;
}

// ---------------------------------------------------------------------------
// BLOQUE -> { LISTA_VARIABLES COMANDOS }
// ---------------------------------------------------------------------------

bool Parser::reconocerBloque() {
    abrirNodo("BLOQUE");
    CierreNodo cierre{this};

    if (!consumir(LLAVE_IZQ)) {
        errorActual("'{' para iniciar el bloque", "BLOQUE",
                    "El cuerpo debe ir entre llaves: { ... }");
        return false;
    }

    const int lineaApertura = flujo[pos - 1].linea;
    bool ok = reconocerListaVariables();
    ok = reconocerComandos() && ok;

    if (consumir(LLAVE_DER)) {
        nodo("BLOQUE: cierre '}' en la línea " + std::to_string(flujo[pos - 1].linea));
    } else {
        const Token& token = actual();
        int lineaError = token.linea;
        int columnaError = token.columna;
        if (lineaError > static_cast<int>(lineas.size()) && !lineas.empty()) {
            lineaError = static_cast<int>(lineas.size());
            columnaError = static_cast<int>(lineas.back().size()) + 1;
        }
        reportar(TipoError::SINTACTICO, lineaError, columnaError,
                 "bloque sin cerrar: se esperaba '}' para cerrar el BLOQUE, pero se encontró " +
                     describirToken(token),
                 "La llave '{' de la línea " + std::to_string(lineaApertura) +
                     " no tiene su '}' correspondiente.");
        ok = false;
    }

    return ok;
}

// ---------------------------------------------------------------------------
// LISTA_VARIABLES -> (DECLARACION)*
// ---------------------------------------------------------------------------

bool Parser::reconocerListaVariables() {
    abrirNodo("LISTA_VARIABLES");
    CierreNodo cierre{this};

    bool ok = true;
    while (esTipoBase(actual().tipo)) {
        ok = reconocerDeclaracion() && ok;
    }
    return ok;
}

// ---------------------------------------------------------------------------
// DECLARACION -> TIPO LISTA_VARIABLES_DECLARACION
// ---------------------------------------------------------------------------

bool Parser::reconocerDeclaracion() {
    abrirNodo("DECLARACION");
    CierreNodo cierre{this};

    const InfoTipo tipo = reconocerTipo();
    bool ok = tipo.valido;
    ok = reconocerListaVariablesDeclaracion(tipo) && ok;
    if (tipo.valido) {
        ++declaraciones;
    }
    return ok;
}

// ---------------------------------------------------------------------------
// LISTA_VARIABLES (de una declaración) -> LISTA_ID ;
// ---------------------------------------------------------------------------

bool Parser::reconocerListaVariablesDeclaracion(const InfoTipo& tipo) {
    abrirNodo("LISTA_VARIABLES");
    CierreNodo cierre{this};

    bool listaOk = true;
    const std::vector<std::string> nombres = reconocerListaId(tipo, listaOk);
    if (nombres.empty()) {
        while (!esFin() && !comprobar(PUNTO_COMA) && !comprobar(LLAVE_DER)) {
            avanzar();
        }
        consumir(PUNTO_COMA);
        return false;
    }

    if (consumir(PUNTO_COMA)) {
        nodo("PUNTO_COMA: ;");
        return listaOk;
    }

    if (comprobar(ASIGNACION)) {
        const Token& encontrado = actual();
        reportar(TipoError::SINTACTICO, encontrado.linea, encontrado.columna,
                 "no se admite inicialización en la declaración en esta etapa; "
                 "se encontró '='",
                 "Declare primero y asigne después: int edad; edad = 20;");
        sincronizar();
        consumir(PUNTO_COMA);
        return false;
    }

    errorPuntoComa("la declaración (LISTA_VARIABLES)");

    // Recuperacion: si el token actual inicia otra declaración o un comando,
    // se deja intacto para que el bloque lo reconozca.
    const bool iniciaOtraCosa =
        esTipoBase(actual().tipo) || comprobar(ID) || comprobar(IF) ||
        comprobar(WHILE) || comprobar(FUNC) || comprobar(SCANF) ||
        comprobar(PRINTLN) || comprobar(RETURN) || comprobar(LLAVE_DER);
    if (!iniciaOtraCosa && !esFin()) {
        sincronizar();
        consumir(PUNTO_COMA);
    }
    return false;
}

// ---------------------------------------------------------------------------
// COMANDOS -> COMANDO*
// ---------------------------------------------------------------------------

bool Parser::reconocerComandos() {
    abrirNodo("COMANDOS");
    CierreNodo cierre{this};

    bool ok = true;
    while (!esFin() && !comprobar(LLAVE_DER)) {
        if (esTipoBase(actual().tipo)) {
            const Token& token = actual();
            reportar(TipoError::SINTACTICO, token.linea, token.columna,
                     "las variables deben declararse antes de los comandos",
                     "Mueva la declaración al inicio del bloque, antes de la primera instrucción.");
            ok = reconocerDeclaracion() && ok;
            ok = false;
        } else {
            ok = reconocerComando() && ok;
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------
// COMANDO -> ASIGNACION | IF | WHILE | LLAMADA | RETURN | SCANF | PRINTLN
// ---------------------------------------------------------------------------

bool Parser::reconocerComando() {
    if (comprobar(ID)) {
        if (siguienteEs(PAR_IZQ)) {
            const Token& token = actual();
            reportar(TipoError::SINTACTICO, token.linea, token.columna,
                     "las llamadas a función deben comenzar con 'func'; se encontró '" +
                         token.lexema + "'",
                     "Ejemplo: func nombre(argumentos);");
            while (!esFin() && !comprobar(PUNTO_COMA) && !comprobar(LLAVE_DER)) {
                avanzar();
            }
            consumir(PUNTO_COMA);
            return false;
        }
        return reconocerAsignacion();
    }
    if (comprobar(IF)) {
        return reconocerIf();
    }
    if (comprobar(WHILE)) {
        return reconocerWhile();
    }
    if (comprobar(FUNC)) {
        return reconocerLlamada(true);
    }
    if (comprobar(SCANF) || comprobar(PRINTLN)) {
        return reconocerLlamadaIncorporada();
    }
    if (comprobar(RETURN)) {
        return reconocerReturn();
    }
    if (comprobar(PUNTO_COMA)) {
        const Token& token = actual();
        reportar(TipoError::SINTACTICO, token.linea, token.columna,
                 "sentencia vacía: se encontró ';' sin una instrucción",
                 "Elimine el ';' sobrante o escriba la instrucción.");
        avanzar();
        return false;
    }
    if (comprobar(LLAVE_IZQ)) {
        const Token& token = actual();
        reportar(TipoError::SINTACTICO, token.linea, token.columna,
                 "bloque inesperado: se esperaba una instrucción",
                 "Los bloques solo aparecen después de main, if, else, while o una función.");
        return reconocerBloque();
    }

    const Token& token = actual();
    reportar(TipoError::SINTACTICO, token.linea, token.columna,
             "token inesperado " + describirToken(token) +
                 "; se esperaba una asignación, if, while, func, return, scanf o println",
             "Revise la línea o la instrucción anterior.");
    avanzar();
    if (comprobar(LLAVE_IZQ)) {
        reconocerBloque();
    } else {
        sincronizar();
        consumir(PUNTO_COMA);
    }
    return false;
}

// ---------------------------------------------------------------------------
// ASIGNACION -> ID ( [ COMPLEMENTO ] )* = COMPLEMENTO ;
// ---------------------------------------------------------------------------

bool Parser::reconocerAsignacion() {
    abrirNodo("ASIGNACION");
    CierreNodo cierre{this};

    if (!comprobar(ID)) {
        errorActual("un identificador al inicio de la asignación", "ASIGNACION");
        return false;
    }

    const Token objetivo = actual();
    avanzar();
    nodo("OBJETIVO: " + objetivo.lexema);

    bool ok = true;
    if (comprobar(CORCH_IZQ)) {
        ok = reconocerSubindices() && ok;
    }

    if (!consumir(ASIGNACION)) {
        const Token& encontrado = actual();
        reportar(TipoError::SINTACTICO, encontrado.linea, encontrado.columna,
                 "se esperaba '=' en la asignación de '" + objetivo.lexema +
                     "', pero se encontró " + describirToken(encontrado),
                 "Formato esperado: variable = expresión;");
        while (!esFin() && !comprobar(PUNTO_COMA) && !comprobar(LLAVE_DER)) {
            avanzar();
        }
        consumir(PUNTO_COMA);
        return false;
    }
    nodo("ASIGNACION: =");

    ok = reconocerComplemento() && ok;

    if (consumir(PUNTO_COMA)) {
        nodo("PUNTO_COMA: ;");
    } else {
        errorPuntoComa("la asignación (ASIGNACION)");
        if (!comprobar(LLAVE_DER) && !esFin()) {
            sincronizar();
            consumir(PUNTO_COMA);
        }
        ok = false;
    }

    ++asignaciones;
    return ok;
}

// ---------------------------------------------------------------------------
// IF -> if ( COMPLEMENTO ) BLOQUE [ else ( IF | BLOQUE ) ]
// ---------------------------------------------------------------------------

bool Parser::reconocerIf() {
    abrirNodo("IF");
    CierreNodo cierre{this};

    bool ok = true;
    if (!consumir(IF)) {
        errorActual("'if'", "IF");
        return false;
    }

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después de 'if'", "IF",
                    "Formato esperado: if (condición) { ... }");
        ok = false;
    }

    if (!comprobar(LLAVE_IZQ)) {
        ok = reconocerComplemento() && ok;
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar la condición de 'if'", "IF");
        ok = false;
        while (!esFin() && !comprobar(PAR_DER) && !comprobar(LLAVE_IZQ)) {
            avanzar();
        }
        consumir(PAR_DER);
    }

    if (comprobar(LLAVE_IZQ)) {
        ok = reconocerBloque() && ok;
    } else {
        errorActual("'{' para iniciar el bloque de 'if'", "IF");
        ok = false;
    }

    if (consumir(ELSE)) {
        if (comprobar(IF)) {
            ok = reconocerIf() && ok;
        } else if (comprobar(LLAVE_IZQ)) {
            ok = reconocerBloque() && ok;
        } else {
            errorActual("'{' o 'if' después de 'else'", "IF",
                        "Formato esperado: else { ... } o else if (...) { ... }");
            ok = false;
        }
    }

    return ok;
}

// ---------------------------------------------------------------------------
// WHILE -> while ( COMPLEMENTO ) BLOQUE
// ---------------------------------------------------------------------------

bool Parser::reconocerWhile() {
    abrirNodo("WHILE");
    CierreNodo cierre{this};

    bool ok = true;
    if (!consumir(WHILE)) {
        errorActual("'while'", "WHILE");
        return false;
    }

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después de 'while'", "WHILE",
                    "Formato esperado: while (condición) { ... }");
        ok = false;
    }

    if (!comprobar(LLAVE_IZQ)) {
        ok = reconocerComplemento() && ok;
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar la condición de 'while'", "WHILE");
        ok = false;
        while (!esFin() && !comprobar(PAR_DER) && !comprobar(LLAVE_IZQ)) {
            avanzar();
        }
        consumir(PAR_DER);
    }

    if (comprobar(LLAVE_IZQ)) {
        ok = reconocerBloque() && ok;
    } else {
        errorActual("'{' para iniciar el bloque de 'while'", "WHILE");
        ok = false;
    }

    return ok;
}

// ---------------------------------------------------------------------------
// LLAMADA -> func ID ( LISTA_ARGUMENTOS ) [ ; ]
// ---------------------------------------------------------------------------

bool Parser::reconocerLlamada(bool consumirPuntoComa) {
    abrirNodo("LLAMADA");
    CierreNodo cierre{this};

    bool ok = true;
    if (!consumir(FUNC)) {
        errorActual("'func'", "LLAMADA");
        return false;
    }

    if (!comprobar(ID)) {
        errorActual("un nombre de función (ID)", "LLAMADA",
                    "Formato esperado: func nombre(argumentos);");
        ok = false;
    } else {
        nodo("ID: " + actual().lexema);
        avanzar();
    }

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después del nombre de la función", "LLAMADA");
        ok = false;
        if (consumirPuntoComa) {
            sincronizar();
            consumir(PUNTO_COMA);
        }
        return false;
    }

    if (!comprobar(PAR_DER)) {
        ok = reconocerListaArgumentos() && ok;
    }
    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar los argumentos de la llamada", "LLAMADA");
        ok = false;
        while (!esFin() && !comprobar(PAR_DER) && !comprobar(PUNTO_COMA) &&
               !comprobar(LLAVE_DER)) {
            avanzar();
        }
        consumir(PAR_DER);
    }

    if (consumirPuntoComa) {
        if (consumir(PUNTO_COMA)) {
            nodo("PUNTO_COMA: ;");
        } else {
            errorPuntoComa("la llamada a función (LLAMADA)");
            ok = false;
        }
    }

    return ok;
}

// ---------------------------------------------------------------------------
// LLAMADA INCORPORADA -> (scanf|println) ( LISTA_ARGUMENTOS ) ;
// ---------------------------------------------------------------------------

bool Parser::reconocerLlamadaIncorporada() {
    abrirNodo("LLAMADA");
    CierreNodo cierre{this};

    bool ok = true;
    const Token palabra = actual();
    nodo("ID: " + palabra.lexema);
    avanzar();

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después de '" + palabra.lexema + "'", "LLAMADA");
        ok = false;
        sincronizar();
        consumir(PUNTO_COMA);
        return false;
    }

    if (!comprobar(PAR_DER)) {
        ok = reconocerListaArgumentos() && ok;
    }
    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar los argumentos de '" + palabra.lexema + "'",
                    "LLAMADA");
        ok = false;
        while (!esFin() && !comprobar(PAR_DER) && !comprobar(PUNTO_COMA) &&
               !comprobar(LLAVE_DER)) {
            avanzar();
        }
        consumir(PAR_DER);
    }

    if (consumir(PUNTO_COMA)) {
        nodo("PUNTO_COMA: ;");
    } else {
        errorPuntoComa("la llamada incorporada (LLAMADA)");
        ok = false;
    }

    return ok;
}

// ---------------------------------------------------------------------------
// RETURN -> return [ COMPLEMENTO ] ;
// ---------------------------------------------------------------------------

bool Parser::reconocerReturn() {
    abrirNodo("RETURN");
    CierreNodo cierre{this};

    bool ok = true;
    if (!consumir(RETURN)) {
        errorActual("'return'", "RETURN");
        return false;
    }

    if (!comprobar(PUNTO_COMA)) {
        ok = reconocerComplemento() && ok;
    }

    if (consumir(PUNTO_COMA)) {
        nodo("PUNTO_COMA: ;");
    } else {
        errorPuntoComa("la sentencia RETURN");
        if (!comprobar(LLAVE_DER) && !esFin()) {
            sincronizar();
            consumir(PUNTO_COMA);
        }
        ok = false;
    }

    return ok;
}

// ---------------------------------------------------------------------------
// LISTA_PARAMETROS -> TIPO ID (DIMENSION*) ( , ... )*
// ---------------------------------------------------------------------------

bool Parser::reconocerListaParametros() {
    abrirNodo("PARAMETROS");
    CierreNodo cierre{this};

    bool ok = true;
    while (true) {
        const InfoTipo tipo = reconocerTipo();
        if (!tipo.valido) {
            ok = false;
            break;
        }

        if (!comprobar(ID)) {
            errorActual("un nombre de parámetro (ID)", "PARAMETROS");
            ok = false;
            break;
        }

        const Token nombre = actual();
        avanzar();

        InfoTipo tipoParametro = tipo;
        while (comprobar(CORCH_IZQ)) {
            if (!reconocerDimension(tipoParametro)) {
                ok = false;
                break;
            }
        }

        nodo("PARAMETRO: " + nombre.lexema + " (" + tipoParametro.completo + ")");

        if (!consumir(COMA)) {
            break;
        }
    }

    return ok;
}

// ---------------------------------------------------------------------------
// LISTA_ARGUMENTOS -> COMPLEMENTO ( , COMPLEMENTO )*
// ---------------------------------------------------------------------------

bool Parser::reconocerListaArgumentos() {
    abrirNodo("ARGUMENTOS");
    CierreNodo cierre{this};

    bool ok = true;
    while (true) {
        ok = reconocerComplemento() && ok;
        if (!consumir(COMA)) {
            break;
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------
// TIPO_RETORNO -> void | TIPO_BASE
// ---------------------------------------------------------------------------

Parser::InfoTipo Parser::reconocerTipoRetorno() {
    return reconocerTipoBase(true);
}

// ---------------------------------------------------------------------------
// TIPO -> TIPO_BASE DIMENSION*
// ---------------------------------------------------------------------------

Parser::InfoTipo Parser::reconocerTipo() {
    abrirNodo("TIPO");
    CierreNodo cierre{this};

    InfoTipo tipo = reconocerTipoBase(false);
    while (tipo.valido && comprobar(CORCH_IZQ)) {
        if (!reconocerDimension(tipo)) {
            break;
        }
    }
    return tipo;
}

// ---------------------------------------------------------------------------
// TIPO_BASE -> int | float | char | boolean   (void solo como retorno)
// ---------------------------------------------------------------------------

Parser::InfoTipo Parser::reconocerTipoBase(bool permitirVoid) {
    InfoTipo tipo;
    const Token& token = actual();
    const bool valido = esTipoBase(token.tipo) || (permitirVoid && token.tipo == VOID);

    if (valido) {
        tipo.base = token.lexema;
        tipo.completo = token.lexema;
        tipo.valido = true;
        nodo((permitirVoid ? "TIPO_RETORNO: " : "TIPO_BASE: ") + token.lexema);
        avanzar();
    } else {
        errorActual(permitirVoid
                        ? "un tipo de retorno ('void', 'int', 'float', 'char', 'boolean')"
                        : "un tipo de dato ('int', 'float', 'char', 'boolean')",
                    "TIPO_BASE");
    }
    return tipo;
}

// ---------------------------------------------------------------------------
// DIMENSION -> [ NUM_INT ]
// ---------------------------------------------------------------------------

bool Parser::reconocerDimension(InfoTipo& tipo) {
    bool ok = consumir(CORCH_IZQ);
    if (!ok) {
        errorActual("'[' para iniciar la DIMENSION", "DIMENSION");
        return false;
    }

    std::string tamano;
    if (comprobar(NUM_INT)) {
        tamano = actual().lexema;
        avanzar();
    } else {
        errorActual("un tamaño entero constante (NUM_INT)", "DIMENSION",
                    "Ejemplo de arreglo: int notas[10];");
        ok = false;
        while (!esFin() && !comprobar(CORCH_DER) && !comprobar(PUNTO_COMA) &&
               !comprobar(LLAVE_DER)) {
            avanzar();
        }
    }

    if (consumir(CORCH_DER)) {
        if (!tamano.empty()) {
            tipo.completo += "[" + tamano + "]";
            ++tipo.dimensiones;
            nodo("DIMENSION: [" + tamano + "]");
        }
    } else {
        errorActual("']' para cerrar la DIMENSION", "DIMENSION",
                    "Cada '[' debe cerrarse con ']'. Ejemplo: int notas[10];");
        ok = false;
    }

    return ok;
}

// ---------------------------------------------------------------------------
// LISTA_ID -> ID DIMENSION* ( , ID DIMENSION* )*
// ---------------------------------------------------------------------------

std::vector<std::string> Parser::reconocerListaId(const InfoTipo& tipo, bool& ok) {
    abrirNodo("LISTA_ID");
    CierreNodo cierre{this};

    std::vector<std::string> nombres;
    ok = true;

    while (true) {
        if (!comprobar(ID)) {
            errorActual("un nombre de variable (ID)", "LISTA_ID",
                        "Ejemplo: int edad, promedio;");
            ok = false;
            break;
        }

        const Token token = actual();
        avanzar();
        nombres.push_back(token.lexema);

        // Dimensiones después del identificador (estilo C): int notas[10];
        // También se aceptan después del tipo (int[10] notas;).
        InfoTipo tipoVariable = tipo;
        while (comprobar(CORCH_IZQ)) {
            if (!reconocerDimension(tipoVariable)) {
                ok = false;
                break;
            }
        }

        Simbolo simbolo;
        simbolo.nombre = token.lexema;
        simbolo.tipo = tipoVariable.completo;
        simbolo.linea = token.linea;
        simbolo.columna = token.columna;
        tabla.insertar(simbolo);  // Solo registro; no se valida semántica.

        nodo("ID: " + token.lexema + " (" + tipoVariable.completo + ")");

        if (!consumir(COMA)) {
            break;
        }
    }

    return nombres;
}

// ---------------------------------------------------------------------------
// Subindices del lado izquierdo: [ COMPLEMENTO ] [ COMPLEMENTO ] ...
// ---------------------------------------------------------------------------

bool Parser::reconocerSubindices() {
    bool ok = true;
    while (comprobar(CORCH_IZQ)) {
        avanzar();
        if (!reconocerComplemento()) {
            ok = false;
            break;
        }
        if (!consumir(CORCH_DER)) {
            errorActual("']' para cerrar el subíndice", "ASIGNACION");
            ok = false;
            break;
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------
// COMPLEMENTO -> OR
// ---------------------------------------------------------------------------

bool Parser::reconocerComplemento() {
    abrirNodo("COMPLEMENTO");
    CierreNodo cierre{this};

    const std::size_t inicio = pos;
    if (!reconocerOr()) {
        return false;
    }

    nodo("EXPRESION: " + textoEntre(inicio, pos));
    return true;
}

// ---------------------------------------------------------------------------
// OR -> AND ( || AND )*
// ---------------------------------------------------------------------------

bool Parser::reconocerOr() {
    if (!reconocerAnd()) {
        return false;
    }
    while (comprobar(OR)) {
        avanzar();
        if (!reconocerAnd()) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// AND -> REL ( && REL )*
// ---------------------------------------------------------------------------

bool Parser::reconocerAnd() {
    if (!reconocerRelacional()) {
        return false;
    }
    while (comprobar(AND)) {
        avanzar();
        if (!reconocerRelacional()) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// REL -> ADD ( COMP ADD )*
// ---------------------------------------------------------------------------

bool Parser::reconocerRelacional() {
    if (!reconocerAditivo()) {
        return false;
    }
    while (comprobar(COMP)) {
        avanzar();
        if (!reconocerAditivo()) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// ADD -> MUL ( (+|-) MUL )*
// ---------------------------------------------------------------------------

bool Parser::reconocerAditivo() {
    if (!reconocerMultiplicativo()) {
        return false;
    }
    while (comprobar(SUMA) || comprobar(RESTA)) {
        avanzar();
        if (!reconocerMultiplicativo()) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// MUL -> UNARIO ( (*|/|%) UNARIO )*
// ---------------------------------------------------------------------------

bool Parser::reconocerMultiplicativo() {
    if (!reconocerUnario()) {
        return false;
    }
    while (comprobar(MULT) || comprobar(DIV) || comprobar(MOD)) {
        avanzar();
        if (!reconocerUnario()) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// UNARIO -> (-|!) UNARIO | PRIMARIO
// ---------------------------------------------------------------------------

bool Parser::reconocerUnario() {
    if (consumir(RESTA) || consumir(NOT)) {
        return reconocerUnario();
    }
    return reconocerPrimario();
}

// ---------------------------------------------------------------------------
// PRIMARIO -> NUM_INT | NUM_DEC | TEXTO | func ID ( args )
//           | ID subindices | ( COMPLEMENTO )
// ---------------------------------------------------------------------------

bool Parser::reconocerPrimario() {
    if (consumir(NUM_INT) || consumir(NUM_DEC) || consumir(TEXTO)) {
        return true;
    }

    if (comprobar(FUNC)) {
        return reconocerLlamada(false);
    }

    if (comprobar(ID)) {
        nodo("ID: " + actual().lexema);
        avanzar();
        while (comprobar(CORCH_IZQ)) {
            avanzar();
            if (!reconocerComplemento()) {
                return false;
            }
            if (!consumir(CORCH_DER)) {
                errorActual("']' para cerrar el subíndice", "COMPLEMENTO");
                return false;
            }
        }
        return true;
    }

    if (consumir(PAR_IZQ)) {
        if (!reconocerComplemento()) {
            return false;
        }
        if (!consumir(PAR_DER)) {
            errorActual("')' para cerrar la expresión", "COMPLEMENTO",
                        "Verifique que cada '(' tenga su ')'.");
            return false;
        }
        return true;
    }

    errorActual("una expresión (número, texto, identificador, func o '(')",
                "COMPLEMENTO",
                "Ejemplo: promedio = (nota1 + nota2) / 2;");
    return false;
}
