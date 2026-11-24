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
    AstNodo raiz;
    const bool ok = reconocerPrograma(raiz);
    if (ok) {
        ast = std::move(raiz);
    }
    return ok && errores.empty();
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

void Parser::sincronizar() {
    while (!esFin() && !comprobar(PUNTO_COMA) && !comprobar(LLAVE_DER)) {
        avanzar();
    }
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

bool Parser::reconocerPrograma(AstNodo& salida) {
    std::vector<AstNodo> funciones;
    if (!reconocerListaFunciones(funciones)) {
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

    AstNodo principal;
    if (!reconocerPrincipal(principal)) {
        return false;
    }

    if (!esFin()) {
        errorActual("fin de archivo después del bloque principal", "PROGRAMA");
        return false;
    }

    AstNodo programa("Programa");
    for (AstNodo& funcion : funciones) {
        programa.agregar(std::move(funcion));
    }
    programa.agregar(std::move(principal));
    salida = std::move(programa);
    return true;
}

// ---------------------------------------------------------------------------
// LISTA_FUNCIONES -> DECLARACION_FUNCION LISTA_FUNCIONES | epsilon
// ---------------------------------------------------------------------------

bool Parser::reconocerListaFunciones(std::vector<AstNodo>& funciones) {
    while (esInicioFuncion()) {
        AstNodo funcion;
        if (!reconocerDeclaracionFuncion(funcion)) {
            return false;
        }
        funciones.push_back(std::move(funcion));
    }
    return true;
}

// ---------------------------------------------------------------------------
// DECLARACION_FUNCION -> TIPO_RETORNO id ( PARAMETROS ) BLOQUE
// ---------------------------------------------------------------------------

bool Parser::reconocerDeclaracionFuncion(AstNodo& funcion) {
    InfoTipo retorno;
    if (!reconocerTipoRetorno(retorno)) {
        return false;
    }

    if (!comprobar(ID)) {
        errorActual("un nombre de función (ID)", "DECLARACION_FUNCION",
                    "Ejemplo: int suma(int a, int b) { ... }");
        return false;
    }
    const Token nombre = actual();
    avanzar();

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después del nombre de la función", "DECLARACION_FUNCION");
        return false;
    }

    std::vector<AstNodo> parametros;
    if (!comprobar(PAR_DER)) {
        if (!reconocerParametros(parametros)) {
            return false;
        }
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar los parámetros de la función",
                    "DECLARACION_FUNCION");
        return false;
    }

    AstNodo bloque;
    if (!reconocerBloque(bloque)) {
        return false;
    }

    AstNodo nodo("Funcion", nombre.lexema);
    nodo.agregar(AstNodo("TipoRetorno", retorno.completo));
    AstNodo listaParametros("Parametros");
    for (AstNodo& parametro : parametros) {
        listaParametros.agregar(std::move(parametro));
    }
    nodo.agregar(std::move(listaParametros));
    nodo.agregar(std::move(bloque));

    funcion = std::move(nodo);
    return true;
}

// ---------------------------------------------------------------------------
// TIPO_RETORNO -> TIPO | void
// ---------------------------------------------------------------------------

bool Parser::reconocerTipoRetorno(InfoTipo& tipo) {
    if (comprobar(VOID)) {
        tipo.completo = "void";
        tipo.valido = true;
        avanzar();
        return true;
    }
    return reconocerTipo(tipo);
}

// ---------------------------------------------------------------------------
// TIPO -> TIPO_BASE DIMENSION
// ---------------------------------------------------------------------------

bool Parser::reconocerTipo(InfoTipo& tipo) {
    if (!reconocerTipoBase(tipo)) {
        return false;
    }
    return reconocerDimension(tipo);
}

bool Parser::reconocerTipoBase(InfoTipo& tipo) {
    if (!esTipoBase(actual().tipo)) {
        errorActual("un tipo base ('char', 'float', 'int', 'boolean')",
                    "TIPO_BASE");
        return false;
    }
    tipo.completo = actual().lexema;
    tipo.valido = true;
    avanzar();
    return true;
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

bool Parser::reconocerParametros(std::vector<AstNodo>& parametros) {
    while (true) {
        InfoTipo tipo;
        if (!reconocerTipo(tipo)) {
            return false;
        }

        if (!comprobar(ID)) {
            errorActual("un nombre de parámetro (ID)", "PARAMETROS");
            return false;
        }
        const Token nombre = actual();
        avanzar();

        if (comprobar(CORCH_IZQ)) {
            errorActual("dimensiones antes del nombre del parámetro", "PARAMETROS",
                        "Ejemplo: int[10] a");
            return false;
        }

        AstNodo parametro("Parametro", nombre.lexema);
        parametro.agregar(AstNodo("Tipo", tipo.completo));
        parametros.push_back(std::move(parametro));

        if (!consumir(COMA)) {
            return true;
        }
    }
}

// ---------------------------------------------------------------------------
// PRINCIPAL -> [void] main ( ) BLOQUE
// ---------------------------------------------------------------------------

bool Parser::reconocerPrincipal(AstNodo& principal) {
    std::string tipoRetorno;
    if (comprobar(VOID) && siguienteEs(MAIN)) {
        tipoRetorno = "void";
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

    AstNodo bloque;
    if (!reconocerBloque(bloque)) {
        return false;
    }

    AstNodo nodo("Principal", tipoRetorno);
    nodo.agregar(std::move(bloque));
    principal = std::move(nodo);
    return true;
}

// ---------------------------------------------------------------------------
// BLOQUE -> { LISTA_VARIABLES COMANDOS }
// ---------------------------------------------------------------------------

bool Parser::reconocerBloque(AstNodo& bloque) {
    if (!consumir(LLAVE_IZQ)) {
        errorActual("'{' para iniciar el bloque", "BLOQUE",
                    "El cuerpo debe ir entre llaves: { ... }");
        return false;
    }

    const int lineaApertura = flujo[pos - 1].linea;

    std::vector<AstNodo> declaraciones;
    if (!reconocerListaVariables(declaraciones)) {
        return false;
    }

    std::vector<AstNodo> comandos;
    if (!reconocerComandos(comandos)) {
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

    AstNodo nodo("Bloque");
    for (AstNodo& declaracion : declaraciones) {
        nodo.agregar(std::move(declaracion));
    }
    for (AstNodo& comando : comandos) {
        nodo.agregar(std::move(comando));
    }
    bloque = std::move(nodo);
    return true;
}

// ---------------------------------------------------------------------------
// LISTA_VARIABLES -> TIPO id LISTA_ID ; LISTA_VARIABLES | epsilon
// ---------------------------------------------------------------------------

bool Parser::reconocerListaVariables(std::vector<AstNodo>& declaraciones) {
    while (esTipoBase(actual().tipo)) {
        AstNodo declaracion;
        if (!reconocerDeclaracion(declaracion)) {
            return false;
        }
        declaraciones.push_back(std::move(declaracion));
    }
    return true;
}

bool Parser::reconocerDeclaracion(AstNodo& declaracion) {
    InfoTipo tipo;
    if (!reconocerTipo(tipo)) {
        return false;
    }

    AstNodo nodo("Declaracion", tipo.completo);
    if (!reconocerListaId(nodo)) {
        return false;
    }

    if (!consumir(PUNTO_COMA)) {
        errorPuntoComa("la declaración (LISTA_VARIABLES)");
        return false;
    }

    declaracion = std::move(nodo);
    return true;
}

// LISTA_ID -> , id LISTA_ID | epsilon
bool Parser::reconocerListaId(AstNodo& declaracion) {
    if (!comprobar(ID)) {
        errorActual("un nombre de variable (ID)", "LISTA_ID",
                    "Ejemplo: int edad, promedio;");
        return false;
    }
    declaracion.agregar(AstNodo("Variable", actual().lexema));
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
        declaracion.agregar(AstNodo("Variable", actual().lexema));
        avanzar();
    }
    return true;
}

// ---------------------------------------------------------------------------
// COMANDOS -> COMANDO COMANDOS | epsilon
// ---------------------------------------------------------------------------

bool Parser::reconocerComandos(std::vector<AstNodo>& comandos) {
    bool ok = true;

    while (!esFin() && !comprobar(LLAVE_DER)) {
        AstNodo comando;
        if (!reconocerComando(comando)) {
            ok = false;
            if (!esFin() && !comprobar(LLAVE_DER)) {
                sincronizar();
                consumir(PUNTO_COMA);
            }
            continue;
        }
        comandos.push_back(std::move(comando));
    }
    return ok;
}

// COMANDO -> LECTURA | ESCRITURA | ASIGNACION | FUNCION
//          | SELECCION | MIENTRAS | RETORNO
bool Parser::reconocerComando(AstNodo& comando) {
    if (comprobar(SCANF)) {
        return reconocerLectura(comando);
    }
    if (comprobar(PRINTLN)) {
        return reconocerEscritura(comando);
    }
    if (comprobar(ID)) {
        return reconocerAsignacion(comando);
    }
    if (comprobar(FUNC)) {
        return reconocerFuncion(comando);
    }
    if (comprobar(IF)) {
        return reconocerSeleccion(comando);
    }
    if (comprobar(WHILE)) {
        return reconocerMientras(comando);
    }
    if (comprobar(RETURN)) {
        return reconocerRetorno(comando);
    }

    errorActual("una lectura, escritura, asignación, llamada, if, while o return",
                "COMANDO");
    return false;
}

// ---------------------------------------------------------------------------
// LECTURA -> scanf ( TERMINO_LECTURA NUEVO_TERMINO_LECTURA ) ;
// ---------------------------------------------------------------------------

bool Parser::reconocerLectura(AstNodo& comando) {
    if (!consumir(SCANF)) {
        errorActual("'scanf'", "LECTURA");
        return false;
    }
    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después de 'scanf'", "LECTURA");
        return false;
    }

    AstNodo lectura("Lectura");

    if (comprobar(PAR_DER)) {
        errorActual("al menos un identificador en 'scanf'", "LECTURA");
        return false;
    }

    while (true) {
        if (!comprobar(ID)) {
            errorActual("un identificador de variable", "TERMINO_LECTURA");
            return false;
        }

        AstNodo termino("Identificador", actual().lexema);
        avanzar();
        if (!reconocerDimension2(termino)) {
            return false;
        }
        lectura.agregar(std::move(termino));

        if (!consumir(COMA)) {
            break;
        }
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar 'scanf'", "LECTURA");
        return false;
    }
    if (!consumir(PUNTO_COMA)) {
        errorPuntoComa("la lectura (LECTURA)");
        return false;
    }

    comando = std::move(lectura);
    return true;
}

// ---------------------------------------------------------------------------
// ESCRITURA -> println ( TERMINO_ESCRITURA NUEVO_TERMINO_ESCRITURA ) ;
// ---------------------------------------------------------------------------

bool Parser::reconocerEscritura(AstNodo& comando) {
    if (!consumir(PRINTLN)) {
        errorActual("'println'", "ESCRITURA");
        return false;
    }
    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después de 'println'", "ESCRITURA");
        return false;
    }

    AstNodo escritura("Escritura");

    if (comprobar(PAR_DER)) {
        errorActual("al menos un término en 'println'", "ESCRITURA");
        return false;
    }

    while (true) {
        if (comprobar(ID)) {
            AstNodo termino("Identificador", actual().lexema);
            avanzar();
            if (!reconocerDimension2(termino)) {
                return false;
            }
            escritura.agregar(std::move(termino));
        } else if (comprobar(NUM_INT) || comprobar(NUM_DEC)) {
            escritura.agregar(AstNodo("Literal", actual().lexema));
            avanzar();
        } else if (comprobar(TEXTO)) {
            escritura.agregar(AstNodo("Texto", actual().lexema));
            avanzar();
        } else {
            errorActual("un identificador, una constante o un texto",
                        "TERMINO_ESCRITURA");
            return false;
        }

        if (!consumir(COMA)) {
            break;
        }
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar 'println'", "ESCRITURA");
        return false;
    }
    if (!consumir(PUNTO_COMA)) {
        errorPuntoComa("la escritura (ESCRITURA)");
        return false;
    }

    comando = std::move(escritura);
    return true;
}

// ---------------------------------------------------------------------------
// SELECCION -> if ( EXPRESION ) BLOQUE SINO
// ---------------------------------------------------------------------------

bool Parser::reconocerSeleccion(AstNodo& comando) {
    if (!consumir(IF)) {
        errorActual("'if'", "SELECCION");
        return false;
    }
    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después de 'if'", "SELECCION");
        return false;
    }

    AstNodo condicion;
    if (!reconocerExpresion(condicion)) {
        return false;
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar la condición de 'if'", "SELECCION");
        return false;
    }

    AstNodo entonces;
    if (!reconocerBloque(entonces)) {
        return false;
    }

    AstNodo seleccion("If");
    AstNodo nodoCondicion("Condicion");
    nodoCondicion.agregar(std::move(condicion));
    seleccion.agregar(std::move(nodoCondicion));

    AstNodo nodoEntonces("Entonces");
    nodoEntonces.agregar(std::move(entonces));
    seleccion.agregar(std::move(nodoEntonces));

    AstNodo sino;
    if (!reconocerSino(sino)) {
        return false;
    }
    if (!sino.hijos.empty()) {
        seleccion.agregar(std::move(sino));
    }

    comando = std::move(seleccion);
    return true;
}

bool Parser::reconocerSino(AstNodo& sino) {
    if (!consumir(ELSE)) {
        return true;
    }

    AstNodo bloque;
    if (!reconocerBloque(bloque)) {
        return false;
    }

    AstNodo nodo("Sino");
    nodo.agregar(std::move(bloque));
    sino = std::move(nodo);
    return true;
}

// ---------------------------------------------------------------------------
// MIENTRAS -> while ( EXPRESION ) BLOQUE
// ---------------------------------------------------------------------------

bool Parser::reconocerMientras(AstNodo& comando) {
    if (!consumir(WHILE)) {
        errorActual("'while'", "MIENTRAS");
        return false;
    }
    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después de 'while'", "MIENTRAS");
        return false;
    }

    AstNodo condicion;
    if (!reconocerExpresion(condicion)) {
        return false;
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar la condición de 'while'", "MIENTRAS");
        return false;
    }

    AstNodo cuerpo;
    if (!reconocerBloque(cuerpo)) {
        return false;
    }

    AstNodo mientras("While");
    AstNodo nodoCondicion("Condicion");
    nodoCondicion.agregar(std::move(condicion));
    mientras.agregar(std::move(nodoCondicion));

    AstNodo nodoCuerpo("Cuerpo");
    nodoCuerpo.agregar(std::move(cuerpo));
    mientras.agregar(std::move(nodoCuerpo));

    comando = std::move(mientras);
    return true;
}

// ---------------------------------------------------------------------------
// ASIGNACION -> id = COMPLEMENTO ;
// ---------------------------------------------------------------------------

bool Parser::reconocerAsignacion(AstNodo& comando) {
    if (!comprobar(ID)) {
        errorActual("un identificador al inicio de la asignación", "ASIGNACION");
        return false;
    }

    AstNodo asignacion("Asignacion", actual().lexema);
    avanzar();

    if (comprobar(CORCH_IZQ)) {
        errorActual("un identificador sin subíndices en la asignación",
                    "ASIGNACION",
                    "La gramática no permite asignar a un elemento de arreglo.");
        return false;
    }

    if (!consumir(ASIGNACION)) {
        errorActual("'=' en la asignación", "ASIGNACION",
                    "Formato esperado: variable = expresión;");
        return false;
    }

    AstNodo complemento;
    if (!reconocerComplemento(complemento)) {
        return false;
    }
    asignacion.agregar(std::move(complemento));

    if (!consumir(PUNTO_COMA)) {
        errorPuntoComa("la asignación (ASIGNACION)");
        return false;
    }

    comando = std::move(asignacion);
    return true;
}

// ---------------------------------------------------------------------------
// COMPLEMENTO -> EXPRESION | FUNCION
// ---------------------------------------------------------------------------

bool Parser::reconocerComplemento(AstNodo& expr) {
    if (comprobar(FUNC)) {
        return reconocerFuncion(expr);
    }
    return reconocerExpresion(expr);
}

// ---------------------------------------------------------------------------
// FUNCION -> func id ( ARGUMENTOS )
// ---------------------------------------------------------------------------

bool Parser::reconocerFuncion(AstNodo& llamada) {
    if (!consumir(FUNC)) {
        errorActual("'func'", "FUNCION");
        return false;
    }

    if (!comprobar(ID)) {
        errorActual("un nombre de función (ID)", "FUNCION");
        return false;
    }

    AstNodo nodo("Llamada", actual().lexema);
    avanzar();

    if (!consumir(PAR_IZQ)) {
        errorActual("'(' después del nombre de la función", "FUNCION");
        return false;
    }

    std::vector<AstNodo> argumentos;
    if (!comprobar(PAR_DER)) {
        if (!reconocerArgumentos(argumentos)) {
            return false;
        }
    }

    if (!consumir(PAR_DER)) {
        errorActual("')' para cerrar los argumentos de la llamada", "FUNCION");
        return false;
    }

    for (AstNodo& argumento : argumentos) {
        nodo.agregar(std::move(argumento));
    }
    llamada = std::move(nodo);
    return true;
}

bool Parser::reconocerArgumentos(std::vector<AstNodo>& argumentos) {
    while (true) {
        AstNodo argumento;
        if (!reconocerExpresion(argumento)) {
            return false;
        }
        argumentos.push_back(std::move(argumento));

        if (!consumir(COMA)) {
            return true;
        }
    }
}

// ---------------------------------------------------------------------------
// RETORNO -> return EXPRESION ;
// ---------------------------------------------------------------------------

bool Parser::reconocerRetorno(AstNodo& comando) {
    if (!consumir(RETURN)) {
        errorActual("'return'", "RETORNO");
        return false;
    }

    AstNodo retorno("Retorno");

    if (comprobar(PUNTO_COMA)) {
        errorActual("una expresión después de 'return'", "RETORNO");
        return false;
    }

    AstNodo expr;
    if (!reconocerExpresion(expr)) {
        return false;
    }
    retorno.agregar(std::move(expr));

    if (!consumir(PUNTO_COMA)) {
        errorPuntoComa("la sentencia RETORNO");
        return false;
    }

    comando = std::move(retorno);
    return true;
}

// ---------------------------------------------------------------------------
// EXPRESION -> EXPRESION_O
// ---------------------------------------------------------------------------

bool Parser::reconocerExpresion(AstNodo& expr) {
    return reconocerExpresionO(expr);
}

// EXPRESION_O -> EXPRESION_Y ( || EXPRESION_Y )*
bool Parser::reconocerExpresionO(AstNodo& expr) {
    std::vector<AstNodo> operandos;
    std::vector<std::string> operadores;

    AstNodo primero;
    if (!reconocerExpresionY(primero)) {
        return false;
    }
    operandos.push_back(std::move(primero));

    while (comprobar(OR)) {
        operadores.push_back(actual().lexema);
        avanzar();
        AstNodo siguiente;
        if (!reconocerExpresionY(siguiente)) {
            return false;
        }
        operandos.push_back(std::move(siguiente));
    }

    AstNodo resultado = std::move(operandos.back());
    for (std::size_t i = operadores.size(); i-- > 0;) {
        AstNodo binaria("Binaria", operadores[i]);
        binaria.agregar(std::move(operandos[i]));
        binaria.agregar(std::move(resultado));
        resultado = std::move(binaria);
    }

    expr = std::move(resultado);
    return true;
}

// EXPRESION_Y -> EXPRESION_RELACIONAL ( && EXPRESION_RELACIONAL )*
bool Parser::reconocerExpresionY(AstNodo& expr) {
    std::vector<AstNodo> operandos;
    std::vector<std::string> operadores;

    AstNodo primero;
    if (!reconocerExpresionRelacional(primero)) {
        return false;
    }
    operandos.push_back(std::move(primero));

    while (comprobar(AND)) {
        operadores.push_back(actual().lexema);
        avanzar();
        AstNodo siguiente;
        if (!reconocerExpresionRelacional(siguiente)) {
            return false;
        }
        operandos.push_back(std::move(siguiente));
    }

    AstNodo resultado = std::move(operandos.back());
    for (std::size_t i = operadores.size(); i-- > 0;) {
        AstNodo binaria("Binaria", operadores[i]);
        binaria.agregar(std::move(operandos[i]));
        binaria.agregar(std::move(resultado));
        resultado = std::move(binaria);
    }

    expr = std::move(resultado);
    return true;
}

// EXPRESION_RELACIONAL -> EXPRESION_ADITIVA [ comp EXPRESION_ADITIVA ]
bool Parser::reconocerExpresionRelacional(AstNodo& expr) {
    AstNodo izquierda;
    if (!reconocerExpresionAditiva(izquierda)) {
        return false;
    }

    if (comprobar(COMP)) {
        const std::string operador = actual().lexema;
        avanzar();
        AstNodo derecha;
        if (!reconocerExpresionAditiva(derecha)) {
            return false;
        }
        AstNodo binaria("Binaria", operador);
        binaria.agregar(std::move(izquierda));
        binaria.agregar(std::move(derecha));
        izquierda = std::move(binaria);
    }

    expr = std::move(izquierda);
    return true;
}

// EXPRESION_ADITIVA -> EXPRESION_MULTIPLICATIVA ( (+|-) ... )*
bool Parser::reconocerExpresionAditiva(AstNodo& expr) {
    std::vector<AstNodo> operandos;
    std::vector<std::string> operadores;

    AstNodo primero;
    if (!reconocerExpresionMultiplicativa(primero)) {
        return false;
    }
    operandos.push_back(std::move(primero));

    while (comprobar(SUMA) || comprobar(RESTA)) {
        operadores.push_back(actual().lexema);
        avanzar();
        AstNodo siguiente;
        if (!reconocerExpresionMultiplicativa(siguiente)) {
            return false;
        }
        operandos.push_back(std::move(siguiente));
    }

    AstNodo resultado = std::move(operandos.back());
    for (std::size_t i = operadores.size(); i-- > 0;) {
        AstNodo binaria("Binaria", operadores[i]);
        binaria.agregar(std::move(operandos[i]));
        binaria.agregar(std::move(resultado));
        resultado = std::move(binaria);
    }

    expr = std::move(resultado);
    return true;
}

// EXPRESION_MULTIPLICATIVA -> FACTOR ( (*|/|%) FACTOR )*
bool Parser::reconocerExpresionMultiplicativa(AstNodo& expr) {
    std::vector<AstNodo> operandos;
    std::vector<std::string> operadores;

    AstNodo primero;
    if (!reconocerFactor(primero)) {
        return false;
    }
    operandos.push_back(std::move(primero));

    while (comprobar(MULT) || comprobar(DIV) || comprobar(MOD)) {
        operadores.push_back(actual().lexema);
        avanzar();
        AstNodo siguiente;
        if (!reconocerFactor(siguiente)) {
            return false;
        }
        operandos.push_back(std::move(siguiente));
    }

    AstNodo resultado = std::move(operandos.back());
    for (std::size_t i = operadores.size(); i-- > 0;) {
        AstNodo binaria("Binaria", operadores[i]);
        binaria.agregar(std::move(operandos[i]));
        binaria.agregar(std::move(resultado));
        resultado = std::move(binaria);
    }

    expr = std::move(resultado);
    return true;
}

// FACTOR -> SIGNO TERMINO | texto | ! FACTOR | ( EXPRESION )
bool Parser::reconocerFactor(AstNodo& expr) {
    if (comprobar(SUMA) || comprobar(RESTA)) {
        const std::string signo = actual().lexema;
        avanzar();

        AstNodo termino;
        if (!reconocerTermino(termino)) {
            return false;
        }

        AstNodo unaria("Unaria", signo);
        unaria.agregar(std::move(termino));
        expr = std::move(unaria);
        return true;
    }

    if (comprobar(TEXTO)) {
        expr = AstNodo("Texto", actual().lexema);
        avanzar();
        return true;
    }

    if (consumir(NOT)) {
        AstNodo factor;
        if (!reconocerFactor(factor)) {
            return false;
        }
        AstNodo unaria("Unaria", "!");
        unaria.agregar(std::move(factor));
        expr = std::move(unaria);
        return true;
    }

    if (consumir(PAR_IZQ)) {
        AstNodo interna;
        if (!reconocerExpresion(interna)) {
            return false;
        }
        if (!consumir(PAR_DER)) {
            errorActual("')' para cerrar la expresión", "FACTOR");
            return false;
        }
        expr = std::move(interna);
        return true;
    }

    return reconocerTermino(expr);
}

// TERMINO -> id DIMENSION2 | CONSTANTE
bool Parser::reconocerTermino(AstNodo& expr) {
    if (comprobar(ID)) {
        AstNodo termino("Identificador", actual().lexema);
        avanzar();
        if (!reconocerDimension2(termino)) {
            return false;
        }
        expr = std::move(termino);
        return true;
    }
    return reconocerConstante(expr);
}

bool Parser::reconocerConstante(AstNodo& expr) {
    if (comprobar(NUM_INT) || comprobar(NUM_DEC)) {
        expr = AstNodo("Literal", actual().lexema);
        avanzar();
        return true;
    }
    errorActual("una constante numérica (NUM_INT o NUM_DEC)", "CONSTANTE");
    return false;
}

// DIMENSION2 -> [ EXPRESION_ADITIVA ] DIMENSION2 | epsilon
bool Parser::reconocerDimension2(AstNodo& termino) {
    while (consumir(CORCH_IZQ)) {
        AstNodo indice;
        if (!reconocerExpresionAditiva(indice)) {
            return false;
        }
        if (!consumir(CORCH_DER)) {
            errorActual("']' para cerrar el subíndice", "DIMENSION2");
            return false;
        }
        termino.agregar(std::move(indice));
        if (termino.tipo == "Identificador") {
            termino.tipo = "AccesoArreglo";
        }
    }
    return true;
}
