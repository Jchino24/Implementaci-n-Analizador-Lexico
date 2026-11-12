# Analizador Léxico y Sintáctico del Lenguaje LP

**Entregable 1 – Implementación inicial (30%)**

Fecha de entrega: 13 de noviembre.

Integrantes:

- Jose David Choquehuanca Calcina
- Jose Carlos Chino Nina

Proyecto C++17 que integra el **analizador léxico** existente con un
**analizador sintáctico recursivo descendente**. El programa lee un archivo
`.lp`, muestra su secuencia de tokens, reconoce la estructura de un programa LP
y reporta los errores sintácticos con línea, columna, fragmento de código y una
sugerencia de corrección.

---

## 1. Funcionalidades del entregable

| Requisito | Dónde se implementa | Cómo se evidencia |
|---|---|---|
| Integración con el lexer | `Parser::Parser` recorre `Lexer::getNextToken()` y construye la secuencia de tokens | Tabla **SECUENCIA DE TOKENS** |
| Reconocimiento de `PROGRAMA` y `PRINCIPAL` | `Parser::reconocerPrograma`, `Parser::reconocerPrincipal` | Nodos `PROGRAMA` y `PRINCIPAL` de la traza |
| Reconocimiento de `BLOQUE` | `Parser::reconocerBloque` | Nodo `BLOQUE` y cierre `}` |
| `TIPO`, `TIPO_BASE`, `DIMENSION` | `Parser::reconocerTipo`, `reconocerTipoBase`, `reconocerDimension` | Nodos `TIPO`, `TIPO_BASE`, `DIMENSION` |
| `LISTA_VARIABLES` y `LISTA_ID` | `Parser::reconocerListaVariables`, `reconocerListaId` | Nodos homónimos y tabla de símbolos |
| `ASIGNACION` y `COMPLEMENTO` | `Parser::reconocerAsignacion`, `reconocerComplemento` | Nodos homónimos y expresión reconocida |
| Errores sintácticos básicos | `errorPuntoComa`, `errorActual`, `sincronizar` | Sección **DIAGNÓSTICO** |
| Tabla de símbolos (valor agregado) | `TablaSimbolos`, llenada en `reconocerListaId` | Tabla **TABLA DE SÍMBOLOS** |

---

## 2. Gramática implementada

```bnf
PROGRAMA         -> TIPO_RETORNO PRINCIPAL BLOQUE
TIPO_RETORNO     -> void | int | float | char | boolean
PRINCIPAL        -> main ( )
BLOQUE           -> { LISTA_SENTENCIAS }
LISTA_SENTENCIAS -> ( DECLARACION | ASIGNACION )*
DECLARACION      -> TIPO LISTA_VARIABLES
TIPO             -> TIPO_BASE DIMENSION*
TIPO_BASE        -> int | float | char | boolean
DIMENSION        -> [ NUM_INT ]
LISTA_VARIABLES  -> LISTA_ID ;
LISTA_ID         -> ID DIMENSION* ( , ID DIMENSION* )*
ASIGNACION       -> ID ( [ COMPLEMENTO ] )* = COMPLEMENTO ;
COMPLEMENTO      -> EXPRESION ( COMP EXPRESION )*
EXPRESION        -> TERMINO ( ( + | - | || ) TERMINO )*
TERMINO          -> FACTOR ( ( * | / | % | && ) FACTOR )*
FACTOR           -> ( - | ! ) FACTOR
                  | NUM_INT | NUM_DEC | TEXTO
                  | ID ( [ COMPLEMENTO ] )*
                  | ( EXPRESION )
```

Notas:

- `COMP` agrupa `>`, `>=`, `<`, `<=`, `!=`, `==`.
- Las dimensiones se aceptan después del identificador
  (`int notas[10];`, estilo C) y también después del tipo
  (`int[10] notas;`). La forma usada en las pruebas es la primera.
- `COMPLEMENTO` admite números enteros y decimales, textos entre comillas
  dobles, identificadores, arreglos, paréntesis y operadores aritméticos,
  lógicos y relacionales.

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
│   ├── TablaSimbolos.h         # Variables declaradas y su tipo
│   ├── Parser.h / Parser.cpp   # Analizador sintáctico recursivo descendente
│   └── main.cpp                # Interfaz de línea de comandos y reportes
├── casos_de_prueba/            # Programas .lp válidos e inválidos
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
| `0` | El programa LP es sintácticamente válido (puede haber advertencias). |
| `1` | Se detectaron errores léxicos o sintácticos. |
| `2` | Error de uso (archivo inexistente, opción inválida, etc.). |

---

## 6. Ejemplo mínimo reconocido

Archivo `casos_de_prueba/valido_basico.lp`:

```c
void main() {
    int edad;
    float promedio;
    edad = 20;
    promedio = 15.5;
}
```

Salida (resumida):

```text
SECUENCIA DE TOKENS (ANÁLISIS LÉXICO)
...
TRAZA DE RECONOCIMIENTO SINTÁCTICO
PROGRAMA
  TIPO_RETORNO: void
  PRINCIPAL
  BLOQUE
    DECLARACION
      TIPO
        TIPO_BASE: int
      LISTA_VARIABLES
        LISTA_ID
          ID: edad (int)
        PUNTO_COMA: ;
    ...
    ASIGNACION
      OBJETIVO: edad
      ASIGNACION: =
      COMPLEMENTO
        EXPRESION: 20
      PUNTO_COMA: ;
...
RESULTADO FINAL: PROGRAMA VÁLIDO
```

La salida completa está en `evidencias/valido_basico.txt`.

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
[ERROR SINTACTICO] Línea 3, columna 5: se esperaba ';' al final de la declaración (LISTA_VARIABLES), pero se encontró 'edad' (ID)
      3 |     edad = 20;
        |     ^
        Sugerencia: Falta ';' al final de la sentencia de la línea 2.

RESULTADO FINAL: PROGRAMA CON ERRORES
```

El parser informa el token encontrado y la línea donde probablemente falta el
punto y coma; además continúa el análisis para detectar más errores.

---

## 8. Errores detectados

| Caso | Mensaje principal | Archivo de prueba |
|---|---|---|
| Falta `;` en declaración o asignación | `se esperaba ';' ...` | `invalido_falta_punto_coma.lp` |
| Falta `}` de cierre del bloque | `bloque sin cerrar: se esperaba '}' ...` | `invalido_llave_sin_cerrar.lp` |
| Falta `)` en `main()` | `se esperaba ')' para cerrar los parámetros ...` | `invalido_parentesis_sin_cerrar.lp` |
| Falta `]` en una dimensión | `se esperaba ']' para cerrar la DIMENSION ...` | `invalido_dimension_sin_cerrar.lp` |
| Token inesperado | `token inesperado ...` | `invalido_token_inesperado.lp` |
| Falta `=` en una asignación | `se esperaba '=' ...` | `invalido_falta_igual.lp` |
| Carácter no reconocido | `carácter no reconocido ...` | `invalido_caracter_no_reconocido.lp` |
| Variable no declarada | Advertencia, no error | (uso de `x = 1;` sin declarar) |
| Variable redeclarada | Advertencia, no error | `int a; int a;` |

Los errores léxicos y sintácticos no detienen el programa en el primer fallo:
`Parser::sincronizar()` avanza hasta `;` o `}` y el bloque continúa, lo que
permite reportar varios problemas en una sola ejecución.

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
4. Guarda la salida completa de cada caso en `evidencias/<nombre>.txt`.
5. Genera `evidencias/resumen.txt` con el resultado de la corrida.

Casos incluidos:

| Archivo | Resultado esperado |
|---|---|
| `valido_basico.lp` | Válido |
| `valido_completo.lp` | Válido (declaraciones múltiples, arreglos, expresiones) |
| `invalido_falta_punto_coma.lp` | Error sintáctico |
| `invalido_dimension_sin_cerrar.lp` | Error sintáctico |
| `invalido_falta_igual.lp` | Error sintáctico |
| `invalido_llave_sin_cerrar.lp` | Error sintáctico |
| `invalido_parentesis_sin_cerrar.lp` | Error sintáctico |
| `invalido_token_inesperado.lp` | Error sintáctico |
| `invalido_caracter_no_reconocido.lp` | Error léxico |

Resultado de la última corrida:

```text
Total de casos:  9
Aprobados:       9
Fallidos:        0
```

---

## 10. Limitaciones y siguientes pasos

Esta entrega cubre únicamente la estructura inicial del lenguaje LP:

- No se reconocen bloques anidados, `if`, `else`, `for`, `while`, `return`,
  `scanf`, `println` ni funciones adicionales.
- No se admite inicialización en la declaración (`int x = 5;`); se debe
  declarar y luego asignar.
- Las dimensiones deben ser constantes enteras (`NUM_INT`).
- No hay verificación de tipos ni de rangos; la tabla de símbolos registra
  nombres y tipos declarados.
- Las constantes de texto usan comillas dobles (`"A"`); las comillas simples
  no forman parte del lexer actual.
- La recuperación de errores es básica (sincronización por `;` y `}`).

Estos puntos quedan como trabajo para las siguientes entregas.

---

## 11. Sustentación

La guía breve con el flujo completo, el reparto sugerido y preguntas frecuentes
está en [`docs/guia-sustentacion.md`](docs/guia-sustentacion.md).
