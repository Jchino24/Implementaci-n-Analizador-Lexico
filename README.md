# Analizador Léxico y Sintáctico – Entregable 1

Integrantes:

- Jose David Choquehuanca Calcina
- Jose Carlos Chino Nina

Implementación del **Entregable 1**: analizador sintáctico por descenso
recursivo que reutiliza el lexer y reconoce la estructura básica de un programa
LP.

## Alcance

- Integración con el lexer: el parser consume la secuencia de tokens.
- `PROGRAMA -> LISTA_FUNCIONES PRINCIPAL`.
- Funciones antes de `main`, con parámetros y bloque.
- `main()` seguido de bloque (`void main()` también se acepta).
- `BLOQUE -> { LISTA_VARIABLES COMANDOS }`, con declaraciones antes de
  comandos.
- Declaraciones: `TIPO`, `TIPO_BASE`, `DIMENSION`, `LISTA_VARIABLES`,
  `LISTA_ID`.
- Asignaciones: `ASIGNACION` y `COMPLEMENTO`.
- Expresiones con precedencia: `||`, `&&`, relacionales, aditivas y
  multiplicativas.
- Detección de errores: `;` faltante, llaves/paréntesis sin cerrar y token
  inesperado.

No se implementan `if`, `while`, `scanf`, `println`, `return`, llamadas a
función, `for`, tabla de símbolos ni análisis semántico.

## Gramática

```bnf
PROGRAMA             -> LISTA_FUNCIONES PRINCIPAL
LISTA_FUNCIONES      -> DECLARACION_FUNCION LISTA_FUNCIONES | ε
DECLARACION_FUNCION  -> TIPO_RETORNO id ( PARAMETROS ) BLOQUE
TIPO_RETORNO         -> TIPO | void
TIPO                 -> TIPO_BASE DIMENSION
TIPO_BASE            -> char | float | int | boolean
DIMENSION            -> [ num_int ] DIMENSION | ε
PARAMETROS           -> TIPO id LISTA_PARAMETROS | ε
LISTA_PARAMETROS     -> , TIPO id LISTA_PARAMETROS | ε
PRINCIPAL            -> [void] main ( ) BLOQUE
BLOQUE               -> { LISTA_VARIABLES COMANDOS }
LISTA_VARIABLES      -> TIPO id LISTA_ID ; LISTA_VARIABLES | ε
LISTA_ID             -> , id LISTA_ID | ε
COMANDOS             -> ASIGNACION COMANDOS | ε
ASIGNACION           -> id = COMPLEMENTO ;
COMPLEMENTO          -> EXPRESION
EXPRESION            -> EXPRESION_O
EXPRESION_O          -> EXPRESION_Y ( || EXPRESION_Y )*
EXPRESION_Y          -> EXPRESION_RELACIONAL ( && EXPRESION_RELACIONAL )*
EXPRESION_RELACIONAL -> EXPRESION_ADITIVA [ comp EXPRESION_ADITIVA ]
EXPRESION_ADITIVA    -> EXPRESION_MULTIPLICATIVA ( (+|-) ... )*
EXPRESION_MULTIPLICATIVA -> FACTOR ( (*|/|%) FACTOR )*
FACTOR               -> SIGNO TERMINO | texto | ! FACTOR | ( EXPRESION )
TERMINO              -> id DIMENSION2 | CONSTANTE
DIMENSION2           -> [ EXPRESION_ADITIVA ] DIMENSION2 | ε
SIGNO                -> + | - | ε
CONSTANTE            -> num_int | num_dec
```

## Estructura

```text
├── CMakeLists.txt
├── README.md
├── src/
│   ├── Token.h / Token.cpp
│   ├── Lexer.h / Lexer.cpp
│   ├── Parser.h / Parser.cpp
│   └── main.cpp
├── casos_de_prueba/          # 2 válidos y 3 inválidos
├── pruebas/ejecutar_pruebas.sh
└── evidencias/
```

## Compilar y ejecutar

```bash
cmake -S . -B build
cmake --build build -j
./build/analizador casos_de_prueba/valido_basico.lp
```

También:

```bash
g++ -std=c++17 -Wall -Wextra -o analizador src/*.cpp
./analizador casos_de_prueba/valido_basico.lp
```

Códigos de salida: `0` válido, `1` con errores, `2` error de uso o archivo.

## Ejemplo válido

`casos_de_prueba/valido_basico.lp`:

```c
void main() {
    int edad;
    float promedio;
    edad = 20;
    promedio = 15.5;
}
```

Salida:

```text
RESULTADO FINAL: PROGRAMA VÁLIDO
```

## Ejemplos inválidos

- `invalido_falta_punto_coma.lp`: falta `;` en la declaración.
- `invalido_llave_sin_cerrar.lp`: falta `}` de cierre.
- `invalido_token_inesperado.lp`: aparece un token que no inicia una asignación.

Ejemplo de mensaje:

```text
[ERROR SINTACTICO] Línea 3: se esperaba ';' al final de la declaración
(LISTA_VARIABLES), pero se encontró 'edad' (ID)
      3 |     edad = 20;
        |     ^
        Sugerencia: Falta ';' al final de la sentencia de la línea 2.
```

## Pruebas y evidencias

```bash
./pruebas/ejecutar_pruebas.sh
```

El script ejecuta los 5 casos, verifica el código de salida y guarda la salida
completa en `evidencias/`. Resultado actual:

```text
Total de casos:  5
Aprobados:       5
Fallidos:        0
```

## Sustentación

Dividir la explicación en cuatro partes:

1. **Lexer:** cómo se forman los tokens.
2. **Parser:** descenso recursivo y funciones por no terminal.
3. **Errores:** línea, fragmento y sugerencia.
4. **Integración:** cómo el parser consume los tokens del lexer.

Punto clave: el parser no lee caracteres; recibe la secuencia de tokens y no
realiza análisis semántico.
