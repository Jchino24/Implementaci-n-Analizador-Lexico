#include "Parser.h"

#include <algorithm>
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

    if (!comprobar(MAIN)) {
        errorActual("'main' (PRINCIPAL)", "PROGRAMA",
                    "El programa debe terminar con main() y su bloque.");
        return false;
    }

    ok = reconocerPrincipal() && ok;

    if (!esFin()) {
        const Token& sobrante = actual();
        reportar(TipoError::SINTACTICO, sobrante.linea, sobrante.columna,
                 "contenido inesperado después del bloque principal: se encontró " +
                     describirToken(sobrante),
                 "Solo puede existir una función main() al final del programa.");
        ok = false;
        while (!esFin()) {
            avanzar();
        }
    }

    return ok;
}

// ---------------------------------------------------------------------------
// LISTA_FUNCIONES -> DECLARACION_FUNCION LISTA_FUNCIONES | epsilon
// ---------------------------------------------------------------------------

bool Parser::reconocerListaFunciones() {
    abrirNodo("LISTA_FUNCIONES");
    CierreNodo cierre{this};

    bool ok = true;
    while (esInicioFuncion()) {
        ok = reconocerDeclaracionFuncion() && ok;
    }
    return ok;
}

// ---------------------------------------------------------------------------
// DECLARACION_FUNCION -> TIPO_RETORNO id ( PARAMETROS ) BLOQUE
// ---------------------------------------------------------------------------

bool Parser::reconocerDeclaracionFuncion() {
    abrirNodo("DECLARACION_FUNCION");
    CierreNodo cierre{this};

    bool ok = true;
    const InfoTipo retorno = reconocerTipoRetorno();
    ok = retorno.valido && ok;

    if (!comprobar(ID)) {
        errorActual("un nombre de función (ID)", "DECLARACION_FUNCION",
                    "Ejemplo: int suma(int a, int b) { ... }");
        return false;
    }

    const Token nombre = actual();
    nodo("id: " + nombre.lexema);
    avanzar();
    ++funciones;

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después del nombre de la función", "DECLARACION_FUNCION",
                    "Formato esperado: tipo nombre(parámetros) { ... }");
        ok = false;
        while (!esFin() && !comprobar(LLAVE_IZQ)) {
            avanzar();
        }
    } else {
        if (!comprobar(PAR_DER)) {
            ok = reconocerParametros() && ok;
        }
        if (!consumir(PAR_DER)) {
            errorActual("')' para cerrar los parámetros de la función",
                        "DECLARACION_FUNCION");
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
        errorActual("'{' para iniciar el bloque de la función",
                    "DECLARACION_FUNCION");
        ok = false;
    }

    return ok;
}

// ---------------------------------------------------------------------------
// TIPO -> TIPO_BASE DIMENSION
// ---------------------------------------------------------------------------

Parser::InfoTipo Parser::reconocerTipo() {
    abrirNodo("TIPO");
    CierreNodo cierre{this};

    InfoTipo tipo = reconocerTipoBase();
    if (tipo.valido) {
        reconocerDimension(tipo);
    }
    return tipo;
}

// TIPO_BASE -> char | float | int | boolean
Parser::InfoTipo Parser::reconocerTipoBase() {
    InfoTipo tipo;
    const Token& token = actual();

    if (esTipoBase(token.tipo)) {
        tipo.base = token.lexema;
        tipo.completo = token.lexema;
        tipo.valido = true;
        nodo("TIPO_BASE: " + token.lexema);
        avanzar();
    } else {
        errorActual("un tipo base ('char', 'float', 'int', 'boolean')",
                    "TIPO_BASE");
    }
    return tipo;
}

// DIMENSION -> [ num_int ] DIMENSION | epsilon
bool Parser::reconocerDimension(InfoTipo& tipo) {
    bool ok = true;

    while (consumir(CORCH_IZQ)) {
        std::string tamano;
        if (comprobar(NUM_INT)) {
            tamano = actual().lexema;
            avanzar();
        } else {
            errorActual("un tamaño entero (NUM_INT)", "DIMENSION",
                        "Ejemplo: int[10] notas;");
            ok = false;
            while (!esFin() && !comprobar(CORCH_DER) &&
                   !comprobar(PUNTO_COMA) && !comprobar(LLAVE_DER)) {
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
                        "Cada '[' debe cerrarse con ']'.");
            ok = false;
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------
// TIPO_RETORNO -> TIPO | void
// ---------------------------------------------------------------------------

Parser::InfoTipo Parser::reconocerTipoRetorno() {
    abrirNodo("TIPO_RETORNO");
    CierreNodo cierre{this};

    if (comprobar(VOID)) {
        InfoTipo tipo;
        tipo.base = "void";
        tipo.completo = "void";
        tipo.valido = true;
        nodo("void");
        avanzar();
        return tipo;
    }

    return reconocerTipo();
}

// ---------------------------------------------------------------------------
// PARAMETROS -> TIPO id LISTA_PARAMETROS | epsilon
// ---------------------------------------------------------------------------

bool Parser::reconocerParametros() {
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
        nodo("PARAMETRO: " + nombre.lexema + " (" + tipo.completo + ")");
        avanzar();

        if (comprobar(CORCH_IZQ)) {
            const Token& corchete = actual();
            reportar(TipoError::SINTACTICO, corchete.linea, corchete.columna,
                     "las dimensiones del parámetro deben ir antes del nombre; "
                     "se encontró '['",
                     "Ejemplo: int[10] a");
            ok = false;
            while (!esFin() && !comprobar(COMA) && !comprobar(PAR_DER) &&
                   !comprobar(LLAVE_IZQ)) {
                avanzar();
            }
        }

        if (!consumir(COMA)) {
            break;
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------
// PRINCIPAL -> main ( ) BLOQUE
// ---------------------------------------------------------------------------

bool Parser::reconocerPrincipal() {
    abrirNodo("PRINCIPAL");
    CierreNodo cierre{this};

    bool ok = true;
    if (!consumir(MAIN)) {
        errorActual("'main'", "PRINCIPAL");
        return false;
    }

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después de 'main'", "PRINCIPAL",
                    "La firma debe ser: main() { ... }");
        ok = false;
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar 'main()'", "PRINCIPAL",
                    "La firma debe ser: main() { ... }");
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
// LISTA_VARIABLES -> TIPO id LISTA_ID ; LISTA_VARIABLES | epsilon
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
// DECLARACION -> TIPO id LISTA_ID ;
// ---------------------------------------------------------------------------

bool Parser::reconocerDeclaracion() {
    abrirNodo("DECLARACION");
    CierreNodo cierre{this};

    const InfoTipo tipo = reconocerTipo();
    bool ok = tipo.valido;

    ok = reconocerListaId(tipo) && ok;

    if (consumir(PUNTO_COMA)) {
        nodo("PUNTO_COMA: ;");
    } else {
        errorPuntoComa("la declaración (LISTA_VARIABLES)");
        ok = false;
        if (!esTipoBase(actual().tipo) && !comprobar(ID) && !comprobar(IF) &&
            !comprobar(WHILE) && !comprobar(FUNC) && !comprobar(SCANF) &&
            !comprobar(PRINTLN) && !comprobar(RETURN) && !comprobar(LLAVE_DER) &&
            !esFin()) {
            sincronizar();
            consumir(PUNTO_COMA);
        }
    }

    if (tipo.valido) {
        ++declaraciones;
    }
    return ok;
}

// ---------------------------------------------------------------------------
// LISTA_ID -> , id LISTA_ID | epsilon
// ---------------------------------------------------------------------------

bool Parser::reconocerListaId(const InfoTipo& tipo) {
    abrirNodo("LISTA_ID");
    CierreNodo cierre{this};

    if (!comprobar(ID)) {
        errorActual("un nombre de variable (ID)", "LISTA_ID",
                    "Ejemplo: int edad, promedio;");
        return false;
    }

    while (true) {
        const Token nombre = actual();
        nodo("id: " + nombre.lexema + " (" + tipo.completo + ")");
        avanzar();

        Simbolo simbolo;
        simbolo.nombre = nombre.lexema;
        simbolo.tipo = tipo.completo;
        simbolo.linea = nombre.linea;
        simbolo.columna = nombre.columna;
        tabla.insertar(simbolo);  // Solo registro; no se valida semántica.

        if (comprobar(CORCH_IZQ)) {
            const Token& corchete = actual();
            reportar(TipoError::SINTACTICO, corchete.linea, corchete.columna,
                     "las dimensiones deben ir antes del identificador; "
                     "se encontró '['",
                     "Ejemplo: int[10] notas;");
            while (!esFin() && !comprobar(PUNTO_COMA) && !comprobar(LLAVE_DER)) {
                avanzar();
            }
            return false;
        }

        if (!consumir(COMA)) {
            break;
        }

        if (!comprobar(ID)) {
            errorActual("un nombre de variable (ID) después de ','", "LISTA_ID");
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// COMANDOS -> COMANDO COMANDOS | epsilon
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
                     "Mueva la declaración al inicio del bloque.");
            ok = reconocerDeclaracion() && ok;
            ok = false;
        } else {
            ok = reconocerComando() && ok;
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------
// COMANDO -> LECTURA | ESCRITURA | ASIGNACION | FUNCION
//          | SELECCION | MIENTRAS | RETORNO
// ---------------------------------------------------------------------------

bool Parser::reconocerComando() {
    if (comprobar(SCANF)) {
        return reconocerLectura();
    }
    if (comprobar(PRINTLN)) {
        return reconocerEscritura();
    }
    if (comprobar(ID)) {
        if (siguienteEs(PAR_IZQ)) {
            const Token& token = actual();
            reportar(TipoError::SINTACTICO, token.linea, token.columna,
                     "las llamadas a función deben comenzar con 'func'; se encontró '" +
                         token.lexema + "'",
                     "Ejemplo: func nombre(argumentos)");
            while (!esFin() && !comprobar(PUNTO_COMA) && !comprobar(LLAVE_DER)) {
                avanzar();
            }
            consumir(PUNTO_COMA);
            return false;
        }
        return reconocerAsignacion();
    }
    if (comprobar(FUNC)) {
        return reconocerFuncion();
    }
    if (comprobar(IF)) {
        return reconocerSeleccion();
    }
    if (comprobar(WHILE)) {
        return reconocerMientras();
    }
    if (comprobar(RETURN)) {
        return reconocerRetorno();
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
                 "; se esperaba lectura, escritura, asignación, func, if, while o return",
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
// LECTURA -> scanf ( TERMINO_LECTURA NUEVO_TERMINO_LECTURA ) ;
// ---------------------------------------------------------------------------

bool Parser::reconocerLectura() {
    abrirNodo("LECTURA");
    CierreNodo cierre{this};

    bool ok = true;
    if (!consumir(SCANF)) {
        errorActual("'scanf'", "LECTURA");
        return false;
    }

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después de 'scanf'", "LECTURA",
                    "Formato esperado: scanf(variable);");
        ok = false;
        sincronizar();
        consumir(PUNTO_COMA);
        return false;
    }

    if (comprobar(PAR_DER)) {
        errorActual("al menos un identificador en 'scanf'", "LECTURA",
                    "scanf requiere una o más variables: scanf(edad);");
        ok = false;
    } else {
        ok = reconocerTerminoLectura() && ok;
        while (consumir(COMA)) {
            ok = reconocerTerminoLectura() && ok;
        }
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar 'scanf'", "LECTURA");
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
        errorPuntoComa("la lectura (LECTURA)");
        ok = false;
    }

    return ok;
}

// TERMINO_LECTURA -> id DIMENSION2
bool Parser::reconocerTerminoLectura() {
    if (!comprobar(ID)) {
        errorActual("un identificador de variable", "TERMINO_LECTURA",
                    "scanf solo admite variables: scanf(edad);");
        return false;
    }
    nodo("id: " + actual().lexema);
    avanzar();
    return reconocerDimension2();
}

// ---------------------------------------------------------------------------
// ESCRITURA -> println ( TERMINO_ESCRITURA NUEVO_TERMINO_ESCRITURA ) ;
// ---------------------------------------------------------------------------

bool Parser::reconocerEscritura() {
    abrirNodo("ESCRITURA");
    CierreNodo cierre{this};

    bool ok = true;
    if (!consumir(PRINTLN)) {
        errorActual("'println'", "ESCRITURA");
        return false;
    }

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después de 'println'", "ESCRITURA",
                    "Formato esperado: println(expresión);");
        ok = false;
        sincronizar();
        consumir(PUNTO_COMA);
        return false;
    }

    if (comprobar(PAR_DER)) {
        errorActual("al menos un término en 'println'", "ESCRITURA",
                    "println requiere un identificador, constante o texto.");
        ok = false;
    } else {
        ok = reconocerTerminoEscritura() && ok;
        while (consumir(COMA)) {
            ok = reconocerTerminoEscritura() && ok;
        }
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar 'println'", "ESCRITURA");
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
        errorPuntoComa("la escritura (ESCRITURA)");
        ok = false;
    }

    return ok;
}

// TERMINO_ESCRITURA -> id DIMENSION2 | CONSTANTE | texto
bool Parser::reconocerTerminoEscritura() {
    if (comprobar(ID)) {
        nodo("id: " + actual().lexema);
        avanzar();
        return reconocerDimension2();
    }
    if (comprobar(NUM_INT) || comprobar(NUM_DEC) || comprobar(TEXTO)) {
        nodo("constante/texto: " + actual().lexema);
        avanzar();
        return true;
    }
    errorActual("un identificador, una constante o un texto", "TERMINO_ESCRITURA",
                "println solo admite id, num_int, num_dec o texto.");
    return false;
}

// ---------------------------------------------------------------------------
// DIMENSION2 -> [ EXPRESION_ADITIVA ] DIMENSION2 | epsilon
// ---------------------------------------------------------------------------

bool Parser::reconocerDimension2() {
    bool ok = true;
    while (consumir(CORCH_IZQ)) {
        if (!reconocerExpresionAditiva()) {
            ok = false;
        }
        if (!consumir(CORCH_DER)) {
            errorActual("']' para cerrar el subíndice", "DIMENSION2");
            ok = false;
            break;
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------
// SELECCION -> if ( EXPRESION ) BLOQUE SINO
// ---------------------------------------------------------------------------

bool Parser::reconocerSeleccion() {
    abrirNodo("SELECCION");
    CierreNodo cierre{this};

    bool ok = true;
    if (!consumir(IF)) {
        errorActual("'if'", "SELECCION");
        return false;
    }

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después de 'if'", "SELECCION",
                    "Formato esperado: if (expresión) { ... }");
        ok = false;
    }

    if (!comprobar(LLAVE_IZQ)) {
        ok = reconocerExpresion() && ok;
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar la condición de 'if'", "SELECCION");
        ok = false;
        while (!esFin() && !comprobar(PAR_DER) && !comprobar(LLAVE_IZQ)) {
            avanzar();
        }
        consumir(PAR_DER);
    }

    if (comprobar(LLAVE_IZQ)) {
        ok = reconocerBloque() && ok;
    } else {
        errorActual("'{' para iniciar el bloque de 'if'", "SELECCION");
        ok = false;
    }

    ok = reconocerSino() && ok;
    return ok;
}

// SINO -> else BLOQUE | epsilon
bool Parser::reconocerSino() {
    if (!consumir(ELSE)) {
        return true;
    }

    abrirNodo("SINO");
    CierreNodo cierre{this};

    if (comprobar(LLAVE_IZQ)) {
        return reconocerBloque();
    }

    errorActual("'{' después de 'else'", "SINO",
                "La gramática solo admite else { ... }; no else if.");
    return false;
}

// ---------------------------------------------------------------------------
// MIENTRAS -> while ( EXPRESION ) BLOQUE
// ---------------------------------------------------------------------------

bool Parser::reconocerMientras() {
    abrirNodo("MIENTRAS");
    CierreNodo cierre{this};

    bool ok = true;
    if (!consumir(WHILE)) {
        errorActual("'while'", "MIENTRAS");
        return false;
    }

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después de 'while'", "MIENTRAS",
                    "Formato esperado: while (expresión) { ... }");
        ok = false;
    }

    if (!comprobar(LLAVE_IZQ)) {
        ok = reconocerExpresion() && ok;
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar la condición de 'while'", "MIENTRAS");
        ok = false;
        while (!esFin() && !comprobar(PAR_DER) && !comprobar(LLAVE_IZQ)) {
            avanzar();
        }
        consumir(PAR_DER);
    }

    if (comprobar(LLAVE_IZQ)) {
        ok = reconocerBloque() && ok;
    } else {
        errorActual("'{' para iniciar el bloque de 'while'", "MIENTRAS");
        ok = false;
    }

    return ok;
}

// ---------------------------------------------------------------------------
// ASIGNACION -> id = COMPLEMENTO ;
// ---------------------------------------------------------------------------

bool Parser::reconocerAsignacion() {
    abrirNodo("ASIGNACION");
    CierreNodo cierre{this};

    if (!comprobar(ID)) {
        errorActual("un identificador al inicio de la asignación", "ASIGNACION");
        return false;
    }

    const Token objetivo = actual();
    nodo("id: " + objetivo.lexema);
    avanzar();

    if (comprobar(CORCH_IZQ)) {
        const Token& corchete = actual();
        reportar(TipoError::SINTACTICO, corchete.linea, corchete.columna,
                 "no se puede asignar a un elemento de arreglo; la asignación "
                 "solo admite un identificador",
                 "Formato de la gramática: id = COMPLEMENTO ;");
        sincronizar();
        consumir(PUNTO_COMA);
        return false;
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

    bool ok = reconocerComplemento();

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
// COMPLEMENTO -> EXPRESION | FUNCION
// ---------------------------------------------------------------------------

bool Parser::reconocerComplemento() {
    abrirNodo("COMPLEMENTO");
    CierreNodo cierre{this};

    const std::size_t inicio = pos;
    bool ok = false;
    if (comprobar(FUNC)) {
        ok = reconocerFuncion();
    } else {
        ok = reconocerExpresion();
    }

    if (ok) {
        nodo("COMPLEMENTO: " + textoEntre(inicio, pos));
    }
    return ok;
}

// ---------------------------------------------------------------------------
// FUNCION -> func id ( ARGUMENTOS )
// ---------------------------------------------------------------------------

bool Parser::reconocerFuncion() {
    abrirNodo("FUNCION");
    CierreNodo cierre{this};

    bool ok = true;
    if (!consumir(FUNC)) {
        errorActual("'func'", "FUNCION");
        return false;
    }

    if (!comprobar(ID)) {
        errorActual("un nombre de función (ID)", "FUNCION",
                    "Formato esperado: func nombre(argumentos)");
        ok = false;
    } else {
        nodo("id: " + actual().lexema);
        avanzar();
    }

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después del nombre de la función", "FUNCION");
        return false;
    }

    if (!comprobar(PAR_DER)) {
        ok = reconocerArgumentos() && ok;
    }
    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar los argumentos de la llamada", "FUNCION");
        ok = false;
        while (!esFin() && !comprobar(PAR_DER) && !comprobar(PUNTO_COMA) &&
               !comprobar(LLAVE_DER)) {
            avanzar();
        }
        consumir(PAR_DER);
    }

    return ok;
}

// ---------------------------------------------------------------------------
// ARGUMENTOS -> EXPRESION NUEVO_ARGUMENTO | epsilon
// ---------------------------------------------------------------------------

bool Parser::reconocerArgumentos() {
    abrirNodo("ARGUMENTOS");
    CierreNodo cierre{this};

    bool ok = reconocerExpresion();
    while (consumir(COMA)) {
        ok = reconocerExpresion() && ok;
    }
    return ok;
}

// ---------------------------------------------------------------------------
// RETORNO -> return EXPRESION ;
// ---------------------------------------------------------------------------

bool Parser::reconocerRetorno() {
    abrirNodo("RETORNO");
    CierreNodo cierre{this};

    bool ok = true;
    if (!consumir(RETURN)) {
        errorActual("'return'", "RETORNO");
        return false;
    }

    if (comprobar(PUNTO_COMA)) {
        errorActual("una expresión después de 'return'", "RETORNO",
                    "La gramática exige return expresión;");
        ok = false;
        avanzar();
        return false;
    }

    ok = reconocerExpresion() && ok;

    if (consumir(PUNTO_COMA)) {
        nodo("PUNTO_COMA: ;");
    } else {
        errorPuntoComa("la sentencia RETORNO");
        if (!comprobar(LLAVE_DER) && !esFin()) {
            sincronizar();
            consumir(PUNTO_COMA);
        }
        ok = false;
    }

    return ok;
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
        avanzar();  // SIGNO
        return reconocerTermino();
    }

    if (consumir(TEXTO)) {
        return true;
    }

    if (consumir(NOT)) {
        return reconocerFactor();
    }

    if (consumir(PAR_IZQ)) {
        const bool ok = reconocerExpresion();
        if (!consumir(PAR_DER)) {
            errorActual("')' para cerrar la expresión", "FACTOR",
                        "Verifique que cada '(' tenga su ')'.");
            return false;
        }
        return ok;
    }

    return reconocerTermino();
}

// TERMINO -> id DIMENSION2 | CONSTANTE
bool Parser::reconocerTermino() {
    if (comprobar(ID)) {
        nodo("id: " + actual().lexema);
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
