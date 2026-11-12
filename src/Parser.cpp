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
    std::stable_sort(advertencias.begin(), advertencias.end(), porPosicion);

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

bool Parser::esFin() const {
    return actual().tipo == FIN_ARCHIVO;
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

bool Parser::iniciaSentencia(TokenType tipo) const {
    return tipo == ID || esTipoBase(tipo);
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

// ---------------------------------------------------------------------------
// Reporte de errores y advertencias
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

void Parser::advertencia(int linea, int columna, const std::string& mensaje,
                         const std::string& pista) {
    if (advertencias.size() >= 100) {
        return;
    }
    ErrorCompilacion aviso;
    aviso.tipo = TipoError::SINTACTICO;
    aviso.linea = linea;
    aviso.columna = columna;
    aviso.mensaje = mensaje;
    aviso.pista = pista;
    advertencias.push_back(std::move(aviso));
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
// PROGRAMA -> TIPO_RETORNO PRINCIPAL BLOQUE
// ---------------------------------------------------------------------------

bool Parser::reconocerPrograma() {
    abrirNodo("PROGRAMA");
    CierreNodo cierre{this};
    bool ok = true;

    if (comprobar(VOID) || esTipoBase(actual().tipo)) {
        const InfoTipo retorno = reconocerTipoBase(true);
        ok = retorno.valido && ok;
    } else {
        errorActual("un tipo de retorno ('void', 'int', ...) antes de 'main'",
                    "PROGRAMA",
                    "La cabecera del programa debe ser: void main() { ... }");
        ok = false;
        if (!comprobar(MAIN)) {
            return false;
        }
    }

    if (!comprobar(MAIN)) {
        errorActual("'main'", "PROGRAMA",
                    "Todo programa LP debe tener una función principal main().");
        return false;
    }

    ok = reconocerPrincipal() && ok;
    ok = reconocerBloque() && ok;

    if (!esFin()) {
        const Token& sobrante = actual();
        reportar(TipoError::SINTACTICO, sobrante.linea, sobrante.columna,
                 "contenido inesperado después del bloque principal: se encontró " +
                     describirToken(sobrante),
                 "Solo puede existir una función main() con su bloque.");
        ok = false;
        while (!esFin()) {
            avanzar();
        }
    }

    return ok;
}

// ---------------------------------------------------------------------------
// PRINCIPAL -> main ( )
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
                    "La firma de main debe ser: void main() { ... }");
        return false;
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar los parámetros de main", "PRINCIPAL",
                    "En esta etapa main() no recibe parámetros.");
        ok = false;
        // Recuperacion: descartar parametros no soportados hasta ')' o '{'.
        while (!esFin() && !comprobar(PAR_DER) && !comprobar(LLAVE_IZQ)) {
            avanzar();
        }
        consumir(PAR_DER);
    }

    return ok;
}

// ---------------------------------------------------------------------------
// BLOQUE -> { LISTA_SENTENCIAS }
// ---------------------------------------------------------------------------

bool Parser::reconocerBloque() {
    abrirNodo("BLOQUE");
    CierreNodo cierre{this};

    if (!consumir(LLAVE_IZQ)) {
        errorActual("'{' para iniciar el bloque", "BLOQUE",
                    "El cuerpo de main debe ir entre llaves: { ... }");
        return false;
    }

    const int lineaApertura = flujo[pos - 1].linea;
    bool ok = true;

    while (!esFin() && !comprobar(LLAVE_DER)) {
        if (esTipoBase(actual().tipo)) {
            ok = reconocerDeclaracion() && ok;
        } else if (comprobar(ID)) {
            ok = reconocerAsignacion() && ok;
        } else if (comprobar(VOID)) {
            const Token& token = actual();
            reportar(TipoError::SINTACTICO, token.linea, token.columna,
                     "tipo 'void' no permitido en una declaración dentro del bloque",
                     "'void' solo se usa en la cabecera del programa.");
            avanzar();
            sincronizar();
            consumir(PUNTO_COMA);
            ok = false;
        } else if (comprobar(PUNTO_COMA)) {
            const Token& token = actual();
            reportar(TipoError::SINTACTICO, token.linea, token.columna,
                     "sentencia vacía: se encontró ';' sin una instrucción",
                     "Elimine el ';' sobrante o escriba la declaración/asignación.");
            avanzar();
            ok = false;
        } else if (comprobar(LLAVE_IZQ)) {
            const Token& token = actual();
            reportar(TipoError::SINTACTICO, token.linea, token.columna,
                     "bloque anidado no permitido en esta etapa",
                     "El Entregable 1 solo admite un bloque para main().");
            avanzar();
            ok = false;
        } else {
            const Token& token = actual();
            reportar(TipoError::SINTACTICO, token.linea, token.columna,
                     "token inesperado " + describirToken(token) +
                         "; se esperaba una declaración o una asignación",
                     "Revise la línea o la sentencia anterior.");
            avanzar();
            sincronizar();
            consumir(PUNTO_COMA);
            ok = false;
        }
    }

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
// DECLARACION -> TIPO LISTA_VARIABLES
// ---------------------------------------------------------------------------

bool Parser::reconocerDeclaracion() {
    abrirNodo("DECLARACION");
    CierreNodo cierre{this};

    const InfoTipo tipo = reconocerTipo();
    bool ok = tipo.valido;
    ok = reconocerListaVariables(tipo) && ok;
    if (tipo.valido) {
        ++declaraciones;
    }
    return ok;
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
// TIPO_BASE -> int | float | char | boolean   (void solo en la cabecera)
// ---------------------------------------------------------------------------

Parser::InfoTipo Parser::reconocerTipoBase(bool permitirVoid) {
    InfoTipo tipo;
    const Token& token = actual();
    const bool valido = esTipoBase(token.tipo) || (permitirVoid && token.tipo == VOID);

    if (valido) {
        tipo.base = token.lexema;
        tipo.completo = token.lexema;
        tipo.valido = true;
        nodo((permitirVoid ? "TIPO_RETORNO: " : "TIPO_BASE: ") +
             token.lexema);
        avanzar();
    } else {
        errorActual(permitirVoid
                        ? "un tipo de dato ('void', 'int', 'float', 'char', 'boolean')"
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
// LISTA_VARIABLES -> LISTA_ID ;
// ---------------------------------------------------------------------------

bool Parser::reconocerListaVariables(const InfoTipo& tipo) {
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
    // Recuperacion: si el token actual inicia otra sentencia, se deja intacto
    // para que el bloque la reconozca (caso clasico: falta ';' en una
    // declaración seguida de una asignación).
    if (!iniciaSentencia(actual().tipo) && !comprobar(LLAVE_DER) && !esFin()) {
        sincronizar();
        consumir(PUNTO_COMA);
    }
    return false;
}

// ---------------------------------------------------------------------------
// LISTA_ID -> ID (, ID)*
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

        // Dimensiones escritas despues del identificador (estilo C):
        //   int notas[10];
        // Tambien se aceptan despues del tipo (estilo Java/LP):
        //   int[10] notas;
        InfoTipo tipoVariable = tipo;
        while (comprobar(CORCH_IZQ)) {
            if (!reconocerDimension(tipoVariable)) {
                ok = false;
                break;
            }
        }

        const Simbolo* previo = tabla.buscar(token.lexema);
        Simbolo simbolo;
        simbolo.nombre = token.lexema;
        simbolo.tipo = tipoVariable.completo;
        simbolo.linea = token.linea;
        simbolo.columna = token.columna;

        if (!tabla.insertar(simbolo)) {
            advertencia(token.linea, token.columna,
                        "la variable '" + token.lexema + "' ya había sido declarada" +
                            (previo != nullptr
                                 ? " en la línea " + std::to_string(previo->linea)
                                 : ""),
                        "Se conserva la primera declaración en la tabla de símbolos.");
        }

        nodo("ID: " + token.lexema + " (" + tipoVariable.completo + ")");

        if (!consumir(COMA)) {
            break;
        }
    }

    return nombres;
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

    if (tabla.buscar(objetivo.lexema) == nullptr) {
        advertencia(objetivo.linea, objetivo.columna,
                    "la variable '" + objetivo.lexema +
                        "' se usa sin haber sido declarada",
                    "Declárela antes de asignarle un valor.");
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
        if (!iniciaSentencia(actual().tipo) && !comprobar(LLAVE_DER) && !esFin()) {
            sincronizar();
            consumir(PUNTO_COMA);
        }
        ok = false;
    }

    ++asignaciones;
    return ok;
}

// ---------------------------------------------------------------------------
// Subindices del lado izquierdo: [ COMPLEMENTO ] [ COMPLEMENTO ] ...
// ---------------------------------------------------------------------------

bool Parser::reconocerSubindices() {
    bool ok = true;
    while (comprobar(CORCH_IZQ)) {
        avanzar();
        if (!reconocerExpresion()) {
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
// COMPLEMENTO -> EXPRESION [ COMP EXPRESION ]
// ---------------------------------------------------------------------------

bool Parser::reconocerComplemento() {
    abrirNodo("COMPLEMENTO");
    CierreNodo cierre{this};

    const std::size_t inicio = pos;
    if (!reconocerExpresion()) {
        return false;
    }

    while (comprobar(COMP)) {
        avanzar();
        if (!reconocerExpresion()) {
            return false;
        }
    }

    nodo("EXPRESION: " + textoEntre(inicio, pos));
    return true;
}

// ---------------------------------------------------------------------------
// EXPRESION -> TERMINO ( (+ | - | ||) TERMINO )*
// ---------------------------------------------------------------------------

bool Parser::reconocerExpresion() {
    if (!reconocerTermino()) {
        return false;
    }

    while (comprobar(SUMA) || comprobar(RESTA) || comprobar(OR)) {
        avanzar();
        if (!reconocerTermino()) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// TERMINO -> FACTOR ( (* | / | % | &&) FACTOR )*
// ---------------------------------------------------------------------------

bool Parser::reconocerTermino() {
    if (!reconocerFactor()) {
        return false;
    }

    while (comprobar(MULT) || comprobar(DIV) || comprobar(MOD) || comprobar(AND)) {
        avanzar();
        if (!reconocerFactor()) {
            return false;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// FACTOR -> (- | !) FACTOR | NUM_INT | NUM_DEC | TEXTO | ID subindices
//           | ( EXPRESION )
// ---------------------------------------------------------------------------

bool Parser::reconocerFactor() {
    if (consumir(RESTA) || consumir(NOT)) {
        return reconocerFactor();
    }

    if (consumir(NUM_INT) || consumir(NUM_DEC) || consumir(TEXTO)) {
        return true;
    }

    if (comprobar(ID)) {
        const Token identificador = actual();
        avanzar();
        if (tabla.buscar(identificador.lexema) == nullptr) {
            advertencia(identificador.linea, identificador.columna,
                        "la variable '" + identificador.lexema +
                            "' se usa sin haber sido declarada",
                        "Declárela antes de utilizarla en una expresión.");
        }
        while (comprobar(CORCH_IZQ)) {
            avanzar();
            if (!reconocerExpresion()) {
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
        if (!reconocerExpresion()) {
            return false;
        }
        if (!consumir(PAR_DER)) {
            errorActual("')' para cerrar la expresión", "COMPLEMENTO",
                        "Verifique que cada '(' tenga su ')'.");
            return false;
        }
        return true;
    }

    errorActual("una expresión (número, texto, identificador o '(')", "COMPLEMENTO",
                "Ejemplo: promedio = (nota1 + nota2) / 2;");
    return false;
}
