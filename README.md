# Analizador Léxico y Sintáctico del Lenguaje LP

**Proyecto integrador – Análisis sintáctico para LP**

Integrantes:

- Jose David Choquehuanca Calcina
- Jose Carlos Chino Nina

Proyecto C++17 que reutiliza el **analizador léxico** de LP y agrega un
**analizador sintáctico recursivo descendente**. El programa lee un archivo
`.lp`, muestra su secuencia de tokens, reconoce programas con funciones,
`main()`, bloques, declaraciones, asignaciones, `if/else`, `while` y llamadas a
función, y reporta los errores con línea, fragmento de código y sugerencia.

---

## 1. Alcance implementado

| Requisito de la especificación | Implementación |
|---|---|
| Reutilizar el lexer, sin analizar caracteres desde el parser | `Parser` consume exclusivamente `Lexer::getNextToken()` |
| Consumir la secuencia de tokens | `Parser` guarda los tokens en `flujo` y avanza con `avanzar()` |
| Descenso recursivo por no terminal | Una función por producción: `reconocerPrograma`, `reconocerFuncion`, `reconocerIf`, etc. |
| Distinguir aceptación de errores | `analizar()` devuelve `true/false`; el proceso sale con `0`, `1` o `2` |
| Separar análisis sintáctico del semántico | No se valida uso, declaración ni compatibilidad de tipos |
| Código modular y comentado | `Lexer`, `Parser`, `TablaSimbolos`, `Token` y `main` separados |
| `PROGRAMA -> LISTA_FUNCIONES PRINCIPAL` | `reconocerListaFunciones` y `reconocerPrincipal` |
| `BLOQUE -> { LISTA_VARIABLES COMANDOS }` | `reconocerBloque` exige declaraciones antes de comandos |
| `if/else` y `while` | `reconocerIf` y `reconocerWhile` |
| Llamada con el terminal `func` | `FUNC` agregado al lexer y usado por `reconocerLlamada` |
| Precedencia de expresiones | `||`, `&&`, relacionales, aditivos y multiplicativos |
| Sin producción de `for` | `for` no se reconoce como comando |

---

## 2. Gramática implementada

```bnf
PROGRAMA         -> LISTA_FUNCIONES PRINCIPAL
LISTA_FUNCIONES  -> FUNCION*
FUNCION          -> TIPO_RETORNO ID ( LISTA_PARAMETROS ) BLOQUE
TIPO_RETORNO     -> void | TIPO_BASE
PRINCIPAL        -> [TIPO_RETORNO] main ( ) BLOQUE
BLOQUE           -> { LISTA_VARIABLES COMANDOS }
LISTA_VARIABLES  -> (TIPO LISTA_ID ;)*
COMANDOS         -> COMANDO*
COMANDO          -> ASIGNACION | IF | WHILE | LLAMADA | RETURN
                  | SCANF | PRINTLN
TIPO             -> TIPO_BASE DIMENSION*
TIPO_BASE        -> int | float | char | boolean
DIMENSION        -> [ NUM_INT ]
LISTA_ID         -> ID DIMENSION* ( , ID DIMENSION* )*
ASIGNACION       -> ID ( [ COMPLEMENTO ] )* = COMPLEMENTO ;
IF               -> if ( COMPLEMENTO ) BLOQUE [ else ( IF | BLOQUE ) ]
WHILE            -> while ( COMPLEMENTO ) BLOQUE
LLAMADA          -> func ID ( LISTA_ARGUMENTOS ) [ ; ]
RETURN           -> return [ COMPLEMENTO ] ;
SCANF/PRINTLN    -> (scanf|println) ( LISTA_ARGUMENTOS ) ;
LISTA_PARAMETROS -> [ TIPO ID (DIMENSION*) ( , ... )* ]
LISTA_ARGUMENTOS -> [ COMPLEMENTO ( , COMPLEMENTO )* ]
COMPLEMENTO      -> OR
OR               -> AND ( || AND )*
AND              -> REL ( && REL )*
REL              -> ADD ( COMP ADD )*
ADD              -> MUL ( ( + | - ) MUL )*
MUL              -> UNARIO ( ( * | / | % ) UNARIO )*
UNARIO           -> ( - | ! ) UNARIO | PRIMARIO
PRIMARIO         -> NUM_INT | NUM_DEC | TEXTO
                  | func ID ( LISTA_ARGUMENTOS )
                  | ID ( [ COMPLEMENTO ] )*
                  | ( COMPLEMENTO )
```

Notas:

- `COMP` agrupa `>`, `>=`, `<`, `<=`, `!=`, `==`.
- `main` puede escribirse como `main()` o `void main()`; también se acepta
  `int main()` para no rechazar programas equivalentes.
- `else if (...)` se admite como extensión práctica del `else BLOQUE`.
- Las dimensiones se aceptan después del identificador
  (`int notas[10];`, estilo C) y después del tipo (`int[10] notas;`).
- `func` es una palabra reservada nueva del lexer, usada para llamar
  funciones: `func suma(2, 3);` o `x = func suma(2, 3);`.
- `scanf` y `println` son comandos incorporados del lenguaje LP.
- `f(void)` y `main(void)` se aceptan como listas de parámetros vacías.
- No se reconoce `for`: la gramática de la especificación no lo incluye.

### Convenciones de tokens

| Terminal de la gramática | Token emitido por el lexer |
|---|---|
| `id` | `ID` |
| `num_int` | `NUM_INT` |
| `num_dec` | `NUM_DEC` |
| `texto` | `TEXTO` |
| `comp` | `COMP` (`>`, `>=`, `<`, `<=`, `!=`, `==`) |
| `func` | `FUNC` (palabra reservada agregada) |
| `int`, `float`, `char`, `boolean`, `void` | `INT`, `FLOAT`, `CHAR`, `BOOLEAN`, `VOID` |
| `main`, `if`, `else`, `while`, `return` | `MAIN`, `IF`, `ELSE`, `WHILE`, `RETURN` |
| `scanf`, `println` | `SCANF`, `PRINTLN` |
| Símbolos `{ } ( ) [ ] ; , =` | `LLAVE_*`, `PAR_*`, `CORCH_*`, `PUNTO_COMA`, `COMA`, `ASIGNACION` |

El parser compara estos tipos de token mediante `comprobar()` y `consumir()`;
no vuelve a analizar caracteres.

---

## 3. Estructura del repositorio

```text
.
├── CMakeLists.txt              # Compilación con CMake
├── Makefile                    # Alternativa de compilación con make
├── README.md                   # Este documento
├── src/
│   ├── Token.h / Token.cpp     # Tipos de token y su nombre legible
│   ├── Lexer.h / Lexer.cpp     # Analizador léxico + errores léxicos
│   ├── TablaSimbolos.h         # Registro de variables declaradas
│   ├── Parser.h / Parser.cpp   # Analizador sintáctico recursivo descendente
│   └── main.cpp                # CLI, reportes y códigos de salida
├── casos_de_prueba/            # 12 programas válidos y 17 inválidos
├── pruebas/
│   └── ejecutar_pruebas.sh     # Ejecuta todos los casos y genera evidencias
├── evidencias/                 # Salidas completas de cada caso
└── docs/
    └── guia-sustentacion.md    # Flujo del sistema y preguntas de sustentación
```

---

## 4. Requisitos y compilación

Requisitos:

- Linux (o WSL) con `g++`/`clang++` y CMake 3.16 o superior.
- `make` (CMake lo usa como generador por defecto).

Compilación recomendada:

```bash
cmake -S . -B build
cmake --build build -j
```

Esto genera el ejecutable `build/analizador`. En CLion basta con abrir el
`CMakeLists.txt` y ejecutar la configuración del target `analizador`.

Alternativa con `make`:

```bash
make            # genera ./analizador
make run        # ejecuta el caso válido mínimo
make test       # ejecuta la suite de pruebas
make clean
```

También se puede compilar directamente sin CMake ni make:

```bash
g++ -std=c++17 -Wall -Wextra -o analizador src/*.cpp
```

---

## 5. Ejecución

```bash
./build/analizador casos_de_prueba/valido_basico.lp
```

Si se omite el archivo, el programa intenta usar
`casos_de_prueba/valido_basico.lp`.

Opciones:

```text
--sin-tokens   Omite la tabla de tokens.
--sin-traza    Omite la traza de reconocimiento sintáctico.
--sin-tabla    Omite la tabla de símbolos.
-h, --ayuda    Muestra la ayuda.
```

Códigos de salida:

| Código | Significado |
|---|---|
| `0` | El programa LP es sintácticamente válido. |
| `1` | Se detectaron errores léxicos o sintácticos. |
| `2` | Error de uso (archivo inexistente, opción inválida, etc.). |

La columna se conserva internamente para ubicar el caret, pero los reportes
muestran solo la línea, según lo solicitado.

---

## 6. Ejemplo con funciones y llamadas

Archivo `casos_de_prueba/valido_funciones.lp`:

```c
int suma(int a, int b) {
    int resultado;
    resultado = a + b;
    return resultado;
}

float promedio(float a, float b) {
    return (a + b) / 2;
}

void main() {
    int x;
    float p;
    x = func suma(4, 6);
    p = func promedio(15.5, 12.25);
    println(x);
    println(p);
}
```

Salida (resumida):

```text
PROGRAMA
  LISTA_FUNCIONES
    FUNCION
      TIPO_RETORNO: int
      ID: suma
      PARAMETROS
        TIPO
          TIPO_BASE: int
        PARAMETRO: a (int)
        ...
      BLOQUE
        LISTA_VARIABLES
          DECLARACION
            ...
        COMANDOS
          ASIGNACION
            ...
          RETURN
            ...
  PRINCIPAL
    TIPO_RETORNO: void
    main ( )
    BLOQUE
      LISTA_VARIABLES
        ...
      COMANDOS
        ASIGNACION
          COMPLEMENTO
            LLAMADA
              ID: suma
              ...
        LLAMADA
          ID: println
          ...
RESULTADO FINAL: PROGRAMA VÁLIDO
```

La salida completa está en `evidencias/valido_funciones.txt`.

---

## 7. Ejemplo incorrecto detectado

Archivo `casos_de_prueba/invalido_falta_punto_coma.lp`:

```c
void main() {
    int edad
    edad = 20;
}
```

Salida:

```text
DIAGNÓSTICO
------------------------------------------------------------------------------
[ERROR SINTACTICO] Línea 3: se esperaba ';' al final de la declaración (LISTA_VARIABLES), pero se encontró 'edad' (ID)
      3 |     edad = 20;
        |     ^
        Sugerencia: Falta ';' al final de la sentencia de la línea 2.

RESULTADO FINAL: PROGRAMA CON ERRORES
```

Otro caso representativo:

```text
void main() {
    int a;
    a = 1;
    int b;      <-- las variables deben declararse antes de los comandos
}
```

---

## 8. Errores detectados

| Caso | Mensaje principal | Archivo de prueba |
|---|---|---|
| Falta `;` en declaración o asignación | `se esperaba ';' ...` | `invalido_falta_punto_coma.lp` |
| Falta `;` en `return` o llamada | `se esperaba ';' ...` | `invalido_return_sin_punto_coma.lp`, `invalido_llamada_sin_punto_coma.lp` |
| Falta `}` de cierre | `bloque sin cerrar ...` | `invalido_llave_sin_cerrar.lp` |
| Falta `)` en `main`, `if` o `while` | `se esperaba ')' ...` | `invalido_parentesis_sin_cerrar.lp`, `invalido_if_sin_parentesis.lp` |
| Falta `]` en una dimensión | `se esperaba ']' ...` | `invalido_dimension_sin_cerrar.lp` |
| Token inesperado | `token inesperado ...` | `invalido_token_inesperado.lp` |
| Declaración después de un comando | `las variables deben declararse antes de los comandos` | `invalido_declaracion_despues_comando.lp` |
| Falta `=` en una asignación | `se esperaba '=' ...` | `invalido_falta_igual.lp` |
| Expresión incompleta | `se esperaba una expresión ...` | `invalido_asignacion_sin_expresion.lp` |
| `if` sin bloque | `'{' para iniciar el bloque de 'if'` | `invalido_if_sin_bloque.lp` |
| Función sin `(` | `'(' después del nombre de la función` | `invalido_funcion_sin_parentesis.lp` |
| Llamada sin `func` | `las llamadas a función deben comenzar con 'func'` | `invalido_llamada_sin_func.lp` |
| Llamada sin nombre | `un nombre de función (ID)` | `invalido_func_sin_nombre.lp` |
| `else` sin `if` | `token inesperado 'else'` | `invalido_else_sin_if.lp` |
| Carácter no reconocido | `carácter no reconocido ...` | `invalido_caracter_no_reconocido.lp` |

Los errores léxicos y sintácticos no detienen el programa en el primer fallo:
`Parser::sincronizar()` avanza hasta `;` o `}` y el bloque continúa, lo que
permite reportar varios problemas en una sola ejecución.

> Por decisión de la especificación, **no se realiza análisis semántico**: no se
> comprueba si una variable fue declarada, si los tipos son compatibles ni si
> una función existe. La tabla de símbolos solo registra declaraciones.

---

## 9. Pruebas automatizadas y evidencias

```bash
./pruebas/ejecutar_pruebas.sh
# o, para forzar la recompilación:
./pruebas/ejecutar_pruebas.sh --recompilar
```

El script:

1. Compila el proyecto si no existe `build/analizador` (o si se usa
   `--recompilar`).
2. Ejecuta todos los `.lp` de `casos_de_prueba/`.
3. Comprueba que los `valido_*.lp` terminen con código `0` y los
   `invalido_*.lp` con código `1`.
4. Verifica además el veredicto impreso (`PROGRAMA VÁLIDO` / `PROGRAMA CON ERRORES`).
5. Guarda la salida completa de cada caso en `evidencias/<nombre>.txt`.
6. Genera `evidencias/resumen.txt` con el resultado de la corrida.

Casos válidos incluidos:

| Archivo | Qué cubre |
|---|---|
| `valido_basico.lp` | Ejemplo mínimo de declaraciones y asignaciones |
| `valido_completo.lp` | Variables múltiples, arreglos, expresiones y texto |
| `valido_funciones.lp` | Funciones con parámetros, `return` y llamadas |
| `valido_if_else.lp` | `if`, `else`, `else if` y operadores lógicos |
| `valido_while.lp` | Ciclo `while` con acumulador |
| `valido_anidado.lp` | Bloques anidados y declaraciones locales |
| `valido_expresiones.lp` | Precedencia, paréntesis, unarios y relacionales |
| `valido_llamadas.lp` | Llamadas como comando y dentro de expresiones |
| `valido_arreglos.lp` | Arreglos y subíndices |
| `valido_sin_variables.lp` | Bloque sin declaraciones |
| `valido_scanf.lp` | `scanf` y `println` |
| `valido_void.lp` | `f(void)` y `main(void)` |

Los 17 casos inválidos cubren errores de puntuación, llaves, paréntesis,
corchetes, declaraciones fuera de orden, funciones, llamadas y expresiones.

Resultado de la última corrida:

```text
Total de casos:  29
Aprobados:       29
Fallidos:        0
```

---

## 10. Limitaciones y decisiones

- **No se reconoce `for`**: la gramática de la especificación no lo incluye.
- **No se construye un AST independiente**: la salida incluye la traza/árbol de
  reconocimiento y el resultado del análisis. La estructura reconoce cada
  producción y puede usarse como base para un AST posterior.
- **No hay análisis semántico**: es una decisión explícita del proyecto; la
  tabla de símbolos solo registra nombres y tipos declarados.
- `if`, `else` y `while` exigen bloque entre llaves; `else if` se admite como
  comodidad.
- Las dimensiones deben ser constantes enteras (`NUM_INT`).
- No se admite inicialización en la declaración (`int x = 5;`); se declara y
  luego se asigna.
- Los comentarios son de una línea (`//`); los textos usan comillas dobles.

---

## 11. Sustentación

La guía breve con el flujo completo, el reparto sugerido y preguntas frecuentes
está en [`docs/guia-sustentacion.md`](docs/guia-sustentacion.md).
