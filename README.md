# Analizador Léxico y Sintáctico del Lenguaje LP

**Entregable 2 – Implementación completa y AST**

Integrantes:

- Jose David Choquehuanca Calcina
- Jose Carlos Chino Nina

Analizador sintáctico por **descenso recursivo** que consume los tokens del
lexer, reconoce la gramática completa de LP, reporta errores y construye un
**AST** (árbol de sintaxis abstracta).

---

## 1. Funcionalidades

- Integración total con el lexer: el parser no lee caracteres.
- `PROGRAMA -> LISTA_FUNCIONES PRINCIPAL`.
- Funciones con tipo de retorno, parámetros y bloque.
- `main()` y `void main()`.
- Bloques con declaraciones antes de comandos.
- Declaraciones con tipos y dimensiones (`int[10] notas;`).
- `scanf` y `println` con sus listas de términos.
- `if/else` y `while`.
- Llamadas `func nombre(args)` y argumentos.
- `return` con expresión.
- Expresiones con precedencia completa.
- AST con nodos de programa, funciones, bloques, declaraciones, comandos y
  expresiones.
- Errores con línea, columna, fragmento, token encontrado y sugerencia.
- Recuperación para reportar más de un error cuando es viable.

---

## 2. Gramática

```bnf
PROGRAMA                -> LISTA_FUNCIONES PRINCIPAL
LISTA_FUNCIONES         -> DECLARACION_FUNCION LISTA_FUNCIONES | ε
DECLARACION_FUNCION     -> TIPO_RETORNO id ( PARAMETROS ) BLOQUE
TIPO_RETORNO            -> TIPO | void
TIPO                    -> TIPO_BASE DIMENSION
TIPO_BASE               -> char | float | int | boolean
DIMENSION               -> [ num_int ] DIMENSION | ε
PARAMETROS              -> TIPO id LISTA_PARAMETROS | ε
LISTA_PARAMETROS        -> , TIPO id LISTA_PARAMETROS | ε
PRINCIPAL               -> [void] main ( ) BLOQUE
BLOQUE                  -> { LISTA_VARIABLES COMANDOS }
LISTA_VARIABLES         -> TIPO id LISTA_ID ; LISTA_VARIABLES | ε
LISTA_ID                -> , id LISTA_ID | ε
COMANDOS                -> COMANDO COMANDOS | ε
COMANDO                 -> LECTURA | ESCRITURA | ASIGNACION | FUNCION
                         | SELECCION | MIENTRAS | RETORNO
LECTURA                 -> scanf ( TERMINO_LECTURA NUEVO_TERMINO_LECTURA ) ;
TERMINO_LECTURA         -> id DIMENSION2
NUEVO_TERMINO_LECTURA   -> , TERMINO_LECTURA NUEVO_TERMINO_LECTURA | ε
DIMENSION2              -> [ EXPRESION_ADITIVA ] DIMENSION2 | ε
ESCRITURA               -> println ( TERMINO_ESCRITURA NUEVO_TERMINO_ESCRITURA ) ;
TERMINO_ESCRITURA       -> id DIMENSION2 | CONSTANTE | texto
NUEVO_TERMINO_ESCRITURA -> , TERMINO_ESCRITURA NUEVO_TERMINO_ESCRITURA | ε
SELECCION               -> if ( EXPRESION ) BLOQUE SINO
SINO                    -> else BLOQUE | ε
MIENTRAS                -> while ( EXPRESION ) BLOQUE
ASIGNACION              -> id = COMPLEMENTO ;
COMPLEMENTO             -> EXPRESION | FUNCION
FUNCION                 -> func id ( ARGUMENTOS )
ARGUMENTOS              -> EXPRESION NUEVO_ARGUMENTO | ε
NUEVO_ARGUMENTO         -> , EXPRESION NUEVO_ARGUMENTO | ε
RETORNO                 -> return EXPRESION ;
EXPRESION               -> EXPRESION_O
EXPRESION_O             -> EXPRESION_Y EXPRESION_O2
EXPRESION_O2            -> || EXPRESION_Y EXPRESION_O2 | ε
EXPRESION_Y             -> EXPRESION_RELACIONAL EXPRESION_Y2
EXPRESION_Y2            -> && EXPRESION_RELACIONAL EXPRESION_Y2 | ε
EXPRESION_RELACIONAL    -> EXPRESION_ADITIVA EXPRESION_RELACIONAL2
EXPRESION_RELACIONAL2   -> comp EXPRESION_ADITIVA | ε
EXPRESION_ADITIVA       -> EXPRESION_MULTIPLICATIVA EXPRESION_ADITIVA2
EXPRESION_ADITIVA2      -> OPERADOR_ADITIVO EXPRESION_MULTIPLICATIVA
                           EXPRESION_ADITIVA2 | ε
OPERADOR_ADITIVO        -> + | -
EXPRESION_MULTIPLICATIVA-> FACTOR EXPRESION_MULTIPLICATIVA2
EXPRESION_MULTIPLICATIVA2 -> OPERADOR_MULTIPLICATIVO FACTOR
                           EXPRESION_MULTIPLICATIVA2 | ε
OPERADOR_MULTIPLICATIVO -> * | / | %
FACTOR                  -> SIGNO TERMINO | texto | ! FACTOR | ( EXPRESION )
TERMINO                 -> id DIMENSION2 | CONSTANTE
SIGNO                   -> + | - | ε
CONSTANTE               -> num_int | num_dec
```

`void main()` se acepta por compatibilidad con el ejemplo del curso.

---

## 3. Estructura

```text
├── CMakeLists.txt
├── README.md
├── src/
│   ├── Ast.h                 # Nodo y salida del AST
│   ├── Token.h / Token.cpp   # Tokens
│   ├── Lexer.h / Lexer.cpp   # Analizador léxico
│   ├── Parser.h / Parser.cpp # Parser + construcción del AST
│   └── main.cpp              # CLI y reportes
├── casos_de_prueba/          # 13 válidos y 22 inválidos
├── pruebas/ejecutar_pruebas.sh
├── evidencias/               # Salidas con tokens, errores y AST
└── docs/
    ├── informe-tecnico.md
    └── guia-sustentacion.md
```

---

## 4. Compilación y ejecución

```bash
cmake -S . -B build
cmake --build build -j
./build/analizador casos_de_prueba/valido_ejemplo_5_2.lp
```

También:

```bash
g++ -std=c++17 -Wall -Wextra -o analizador src/*.cpp
```

Códigos de salida: `0` válido, `1` con errores, `2` error de uso o archivo.

La salida incluye:

1. Secuencia de tokens.
2. Diagnóstico de errores.
3. AST cuando el programa es válido.
4. Resultado final.

---

## 5. Ejemplo integral (5.2)

```c
void main() {
    int edad;
    float promedio;
    edad = 20;
    promedio = 15.5;
    if (edad >= 18 && edad <= 60) {
        println("Edad valida");
    } else {
        println("Edad no valida");
    }
    while (edad > 0) {
        edad = edad - 1;
    }
    return edad;
}
```

Fragmento del AST:

```text
Programa
  Principal: void
    Bloque
      Declaracion: int
        Variable: edad
      ...
      If
        Condicion
          Binaria: &&
            Binaria: >=
              Identificador: edad
              Literal: 18
            ...
        Entonces
          Bloque
            Escritura
              Texto: Edad valida
        Sino
          Bloque
            Escritura
              Texto: Edad no valida
      While
        ...
      Retorno
        Identificador: edad
```

---

## 6. AST

`AstNodo` tiene `tipo`, `valor` e `hijos`. Los tipos principales son:

| Nodo | Uso |
|---|---|
| `Programa` | Raíz |
| `Funcion` | Declaración de función |
| `Principal` | `main` |
| `Bloque` | Lista de declaraciones y comandos |
| `Declaracion` / `Variable` | Variables y tipos |
| `Asignacion` | `id = ...;` |
| `If` | Condición, `Entonces` y `Sino` |
| `While` | Condición y `Cuerpo` |
| `Llamada` | `func id(args)` |
| `Retorno` | `return expr;` |
| `Lectura` / `Escritura` | `scanf` / `println` |
| `Binaria` / `Unaria` | Operadores |
| `Literal` / `Texto` / `Identificador` | Operandos |
| `AccesoArreglo` | `id[expr]` |

---

## 7. Errores

El parser reporta:

- `;` faltante.
- `}` sin cerrar.
- `)` sin cerrar.
- `]` sin cerrar.
- Token inesperado.
- Declaración después de un comando.
- `return` sin expresión.
- `scanf` con constante.
- `println` con expresión no permitida.
- Asignación a un elemento de arreglo.
- Llamada sin `func` o sin paréntesis.

Cuando es viable, `sincronizar()` avanza hasta `;` o `}` y continúa para
reportar más de un error.

---

## 8. Pruebas y evidencias

```bash
./pruebas/ejecutar_pruebas.sh
```

- 13 casos válidos.
- 22 casos inválidos.
- 35 en total.

Resultado actual:

```text
Total de casos:  35
Aprobados:       35
Fallidos:        0
```

Cada evidencia en `evidencias/` incluye tokens, AST (si es válido) y errores.

---

## 9. Documentación

- [Informe técnico](docs/informe-tecnico.md)
- [Guía de sustentación](docs/guia-sustentacion.md)
