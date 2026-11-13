# Analizador Léxico y Sintáctico del Lenguaje LP

**Proyecto integrador – Gramática LL(1) de LP**

Integrantes:

- Jose David Choquehuanca Calcina
- Jose Carlos Chino Nina

Analizador sintáctico por **descenso recursivo** que consume los tokens del
lexer y reconoce exactamente la gramática del documento
`Gramatica_LP_LL1.docx`. No vuelve a leer caracteres ni realiza análisis
semántico.

---

## 1. Alcance

| Requisito | Implementación |
|---|---|
| Reutilizar el lexer | `Parser` consume `Lexer::getNextToken()` |
| Consumir la secuencia de tokens | `flujo`, `pos`, `avanzar()` |
| Descenso recursivo | Una función por no terminal |
| `PROGRAMA -> LISTA_FUNCIONES PRINCIPAL` | `reconocerPrograma` |
| Funciones antes de `main` | `reconocerListaFunciones` |
| `main() + BLOQUE` | `reconocerPrincipal` |
| Declaraciones antes de comandos | `reconocerBloque` |
| `if/else` y `while` | `reconocerSeleccion`, `reconocerMientras` |
| Llamada con `func` | `FUNC` en el lexer y `reconocerFuncion` |
| Precedencia de expresiones | `OR -> AND -> REL -> ADD -> MUL -> FACTOR` |
| Sin `for` | No existe producción para `FOR` |
| Sintaxis separada de semántica | No se valida uso, tipos ni funciones |

---

## 2. Gramática implementada

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
PRINCIPAL               -> main ( ) BLOQUE
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

Consecuencias importantes:

- La gramática define `PRINCIPAL -> main ( ) BLOQUE`. El ejemplo del curso usa
  `void main()`; el parser acepta `void` opcional antes de `main` por
  compatibilidad.
- **Las dimensiones van antes del identificador**: `int[10] notas;`, no
  `int notas[10];`.
- **`func` no lleva `;` como comando**: `func saludar()`; el `;` pertenece a
  `ASIGNACION` cuando la llamada es el complemento (`x = func f();`).
- **`return` exige expresión**: `return;` no es válido.
- **`scanf` solo admite variables** (`scanf(edad);`).
- **`println` solo admite id, constante o texto** (`println(x, 20, "hola");`).
- **No se puede asignar a un elemento de arreglo**: `notas[0] = 1;` no está en
  la gramática.
- **No hay `else if`**: solo `else { ... }`.
- **No hay `for`**.

---

## 3. Estructura

```text
.
├── CMakeLists.txt
├── Makefile
├── README.md
├── src/
│   ├── Token.h / Token.cpp
│   ├── Lexer.h / Lexer.cpp
│   ├── TablaSimbolos.h
│   ├── Parser.h / Parser.cpp
│   └── main.cpp
├── casos_de_prueba/          # 12 válidos y 22 inválidos
├── pruebas/ejecutar_pruebas.sh
├── evidencias/
└── docs/guia-sustentacion.md
```

---

## 4. Compilación

```bash
cmake -S . -B build
cmake --build build -j
```

Alternativa:

```bash
make
# o
g++ -std=c++17 -Wall -Wextra -o analizador src/*.cpp
```

---

## 5. Ejecución

```bash
./build/analizador casos_de_prueba/valido_basico.lp
```

Opciones: `--sin-tokens`, `--sin-traza`, `--sin-tabla`, `-h`.

Códigos de salida:

| Código | Significado |
|---|---|
| `0` | Programa sintácticamente válido |
| `1` | Errores léxicos o sintácticos |
| `2` | Error de uso o de archivo |

---

## 6. Ejemplo válido

```c
int suma(int a, int b) {
    int c;
    c = a + b;
    return c;
}

main() {
    int[5] notas;
    int x;
    x = 1;
    x = func suma(2, 3);
    if (x > 0) {
        println("positivo", x, notas[0]);
    } else {
        println(0);
    }
    while (x < 10) {
        x = x + 1;
    }
    func suma(x, 1)
}
```

La traza muestra:

```text
PROGRAMA
  LISTA_FUNCIONES
    DECLARACION_FUNCION
      TIPO_RETORNO
        TIPO
          TIPO_BASE: int
      id: suma
      PARAMETROS
        ...
      BLOQUE
        ...
  PRINCIPAL
    main ( )
    BLOQUE
      LISTA_VARIABLES
        ...
      COMANDOS
        ...
```

La salida completa de cada caso está en `evidencias/`.

---

## 7. Ejemplo inválido

Archivo `casos_de_prueba/invalido_falta_punto_coma.lp`:

```c
main() {
    int edad
    edad = 20;
}
```

Salida:

```text
[ERROR SINTACTICO] Línea 3: se esperaba ';' al final de la declaración
(LISTA_VARIABLES), pero se encontró 'edad' (ID)
      3 |     edad = 20;
        |     ^
        Sugerencia: Falta ';' al final de la sentencia de la línea 2.
```

---

## 8. Errores detectados

| Caso | Archivo de prueba |
|---|---|
| Falta `;` | `invalido_falta_punto_coma.lp` |
| Falta `}` | `invalido_llave_sin_cerrar.lp` |
| Falta `)` | `invalido_parentesis_sin_cerrar.lp` |
| Falta `]` en dimensión | `invalido_dimension_sin_cerrar.lp` |
| Dimensión después del identificador | `invalido_dimension_despues_id.lp` |
| Declaración después de comando | `invalido_declaracion_despues_comando.lp` |
| Falta `=` | `invalido_falta_igual.lp` |
| Expresión incompleta | `invalido_asignacion_sin_expresion.lp` |
| `return` sin expresión | `invalido_return_sin_expresion.lp` |
| `return` sin `;` | `invalido_return_sin_punto_coma.lp` |
| `scanf` con constante | `invalido_scanf_constante.lp` |
| `println` con expresión | `invalido_escritura_expresion.lp` |
| Asignación a arreglo | `invalido_asignacion_arreglo.lp` |
| Llamada sin `func` | `invalido_llamada_sin_func.lp` |
| Llamada sin `(` | `invalido_llamada_sin_parentesis.lp` |
| `else` sin `if` | `invalido_else_sin_if.lp` |
| Token inesperado | `invalido_token_inesperado.lp` |
| Carácter no reconocido | `invalido_caracter_no_reconocido.lp` |

`Parser::sincronizar()` permite reportar más de un error por ejecución.

---

## 9. Pruebas

```bash
./pruebas/ejecutar_pruebas.sh
# o
./pruebas/ejecutar_pruebas.sh --recompilar
```

El script compila, ejecuta los 34 casos, compara código de salida y veredicto,
y guarda `evidencias/<caso>.txt` y `evidencias/resumen.txt`.

Resultado actual:

```text
Total de casos:  34
Aprobados:       34
Fallidos:        0
```

Casos válidos: básico, completo, funciones, `if/else`, `while`, anidado,
expresiones, llamadas, arreglos, sin variables, `scanf` y dimensiones.

---

## 10. Decisiones y limitaciones

- Se sigue la gramática del docx: `main()` (con `void` opcional por compatibilidad),
  dimensiones antes del identificador, `func` sin `;`, `return` con expresión,
  `scanf/println` restringidos y sin `else if` ni `for`.
- No se realiza análisis semántico: no se comprueba declaración, tipos ni
  existencia de funciones.
- La tabla de símbolos solo registra declaraciones.
- No se construye un AST independiente; se muestra la traza/árbol de
  reconocimiento y el resultado.
- Los comentarios son de una línea (`//`) y los textos usan comillas dobles.

---

## 11. Guía de sustentación

[`docs/guia-sustentacion.md`](docs/guia-sustentacion.md)
