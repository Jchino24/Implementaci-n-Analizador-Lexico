# Guía completa de sustentación – Analizador sintáctico LP

Esta guía explica **qué pide la profesora**, **qué se agregó** en la última
actualización, **cómo funciona cada parte del parser** y **cómo defenderlo**.
Está pensada para estudiar y para tenerla abierta durante la exposición.

---

## 0. Resumen en una frase

La profesora pide un **analizador sintáctico por descenso recursivo** que
consuma los tokens producidos por el lexer y reconozca la estructura de un
programa LP completo —funciones, `main`, bloques, declaraciones, asignaciones,
`if/else`, `while` y llamadas con `func`— reportando errores sintácticos, sin
meterse en análisis semántico.

---

## 1. Cuál es el objetivo real de la profesora

### 1.1 Lo que pide explícitamente

| Requisito de la especificación | Qué significa en la práctica | Dónde está en el proyecto |
|---|---|---|
| Reutilizar el lexer | El parser no vuelve a leer caracteres: pide tokens | `Parser::Parser` usa `Lexer::getNextToken()` |
| Consumir la secuencia de tokens | Avanzar por un vector de tokens | `flujo`, `pos`, `avanzar()` |
| Descenso recursivo | Una función por no terminal | `reconocerPrograma`, `reconocerIf`, etc. |
| Distinguir aceptación de errores | Saber si el programa es válido o no | `analizar()` y códigos de salida |
| Separar sintaxis de semántica | No validar variables ni tipos | Se eliminaron las advertencias semánticas |
| Código modular y comentado | Archivos y funciones claras | `Lexer`, `Parser`, `TablaSimbolos`, `Token` |
| `PROGRAMA -> LISTA_FUNCIONES PRINCIPAL` | Funciones antes de `main` | `reconocerPrograma` |
| `PRINCIPAL -> main() BLOQUE` | `main` seguido de su bloque | `reconocerPrincipal` |
| `BLOQUE -> { LISTA_VARIABLES COMANDOS }` | Declarar antes de ejecutar | `reconocerBloque` |
| `if/else` y `while` | Estructuras de control | `reconocerIf`, `reconocerWhile` |
| Llamada con terminal `func` | `func nombre(args)` | `FUNC` en el lexer y `reconocerLlamada` |
| Precedencia de expresiones | `||`, `&&`, relacionales, aditivos y multiplicativos | Cadena de funciones de expresión |
| No incluir `for` | `for` no es un comando válido | No hay producción para `FOR` |

### 1.2 Lo que la profesora NO está pidiendo

- **No** pide ejecutar el programa LP.
- **No** pide comprobar declaración de variables, tipos ni existencia de
  funciones: eso es análisis semántico y queda fuera.
- **No** pide implementar `for`.
- **No** pide una tabla de símbolos con validaciones; la nuestra solo registra.
- **No** pide un AST obligatorio. La especificación muestra el flujo
  “Resultado / AST / errores”; nosotros entregamos **resultado + árbol de
  reconocimiento (traza) + errores**. Si se pidiera un AST formal, se puede
  construir sobre la misma estructura, pero no es un requisito técnico de la
  lista.

### 1.3 La idea central que hay que saber explicar

> “El lexer convierte el texto en tokens. El parser toma esa lista de tokens y,
> con una función por cada no terminal de la gramática, decide si la secuencia
> forma un programa LP válido. Si no, reporta dónde y por qué. No analiza
> caracteres ni valida semántica.”

### 1.4 Checklist para la entrega

- [x] El parser consume tokens del lexer, no caracteres.
- [x] Una función por no terminal.
- [x] `PROGRAMA -> LISTA_FUNCIONES PRINCIPAL`.
- [x] Funciones con tipo de retorno, parámetros y bloque.
- [x] `main()` seguido de bloque.
- [x] `BLOQUE` con declaraciones antes de comandos.
- [x] `if/else` y `while`.
- [x] Llamadas con `func`.
- [x] Expresiones con precedencia.
- [x] No se reconoce `for`.
- [x] Errores con línea, fragmento y sugerencia.
- [x] Sin análisis semántico.
- [x] README, guía, 29 casos y evidencias.

---

## 2. Qué se agregó en esta actualización

| Commit | Qué cambió | Para qué |
|---|---|---|
| `357251f` | Se quitó la columna de las tablas y del encabezado de errores | Salida más limpia; la columna queda interna para el caret |
| `dcace48` | Se agregó `func` al lexer | Terminal que exige la producción de llamada |
| `f2d62e4` | Gramática completa | Funciones, `main`, bloque, `if/else`, `while`, llamadas, precedencia |
| `db4a16f` | 28 casos de prueba | Validar todo lo anterior |
| `c5e579f` | README y guía | Documentar el alcance real |
| `560e7fa` | Etiqueta del resumen | Quitar referencias al entregable anterior |
| `9bc7d96` | `f(void)` y `main(void)` | Compatibilidad práctica |

Antes de esta actualización el proyecto solo reconocía `void main()`, bloques,
declaraciones y asignaciones simples. Ahora reconoce el programa LP completo
que describe la especificación.

---

## 3. Arquitectura del proyecto

```text
main.cpp
  └── Parser
        ├── Lexer            (produce tokens)
        ├── TablaSimbolos    (registra declaraciones, sin validar)
        └── Errores          (lista de errores con línea/columna)

Token.h / Token.cpp   -> tipos de token y nombres legibles
Lexer.h / Lexer.cpp   -> analizador léxico y errores léxicos
Parser.h / Parser.cpp -> analizador sintáctico recursivo descendente
```

Regla de oro:

```text
Lexer:  caracteres -> tokens
Parser: tokens     -> estructura (o errores)
```

El parser **nunca** llama a `getline`, `get`, `peek` ni lee caracteres para
decidir la gramática. Sí lee las líneas del archivo una vez para poder mostrar
el fragmento de código y el caret cuando hay un error; eso es solo diagnóstico.

---

## 4. Flujo completo de una ejecución

```text
1. main.cpp lee los argumentos y obtiene la ruta del .lp.
2. Crea Parser(ruta).
3. Parser abre el archivo y guarda sus líneas (para los mensajes).
4. Parser crea un Lexer(ruta).
5. Parser llama a Lexer::getNextToken() hasta FIN_ARCHIVO.
   - Guarda TODOS los tokens en `tokens` (para la tabla).
   - Guarda los tokens útiles en `flujo` (sin DESCONOCIDO).
6. Parser::analizar() llama a reconocerPrograma().
7. reconocerPrograma():
   - reconocerListaFunciones()  -> funciones antes de main
   - reconocerPrincipal()       -> main + bloque
8. Si no quedan tokens, el programa es válido.
9. main.cpp imprime tokens, traza, tabla y diagnóstico.
10. Devuelve 0 (válido), 1 (errores) o 2 (problema de uso/archivo).
```

Punto importante: `tokens` y `flujo` son distintos.

- `tokens`: todo lo que produjo el lexer, incluidos los `DESCONOCIDO`.
- `flujo`: lo que consume el parser. Excluye `DESCONOCIDO` (ya reportado como
  error léxico) y termina en `FIN_ARCHIVO`.

---

## 5. Cómo funciona el parser por dentro

### 5.1 Estado del parser

| Elemento | Función |
|---|---|
| `flujo` | Vector de tokens que se está analizando |
| `pos` | Índice del token actual |
| `actual()` | Devuelve el token actual |
| `siguiente()` | Devuelve el siguiente token (lookahead de 1) |
| `avanzar()` | Pasa al siguiente token |
| `comprobar(tipo)` | Pregunta si el actual es de ese tipo |
| `consumir(tipo)` | Si coincide, avanza y devuelve `true`; si no, `false` |
| `esFin()` | Indica si se llegó a `FIN_ARCHIVO` |

Ejemplo:

```cpp
if (consumir(PUNTO_COMA)) {
    nodo("PUNTO_COMA: ;");
} else {
    errorPuntoComa("la asignación");
}
```

### 5.2 Traza o árbol de reconocimiento

Cada producción abre un nodo y lo cierra al salir:

```cpp
abrirNodo("IF");
CierreNodo cierre{this};   // cierra el nodo automáticamente al salir
...
nodo("COMPLEMENTO ...");
```

`CierreNodo` es un RAII: aunque la función salga por un `return` anticipado o
por un error, el nodo se cierra. La salida de la traza es indentada y muestra
la jerarquía:

```text
PROGRAMA
  LISTA_FUNCIONES
    FUNCION
      TIPO_RETORNO: int
      ID: suma
      PARAMETROS
        PARAMETRO: a (int)
      BLOQUE
        ...
  PRINCIPAL
    main ( )
    BLOQUE
      ...
```

### 5.3 Errores

| Función | Uso |
|---|---|
| `reportar()` | Guarda un error con línea, columna, mensaje y pista |
| `errorActual(esperado, contexto)` | “Se esperaba X, se encontró Y” |
| `errorPuntoComa(contexto)` | Mensaje específico para `;` faltante |
| `sincronizar()` | Avanza hasta `;` o `}` para seguir analizando |

`analizar()` devuelve `true` solo si `reconocerPrograma()` fue exitoso **y** no
hay errores acumulados. Por eso un error léxico también invalida el programa.

---

## 6. Cada producción explicada

### 6.1 PROGRAMA

**Gramática:**

```bnf
PROGRAMA -> LISTA_FUNCIONES PRINCIPAL
```

**Código:** `Parser::reconocerPrograma()` — `src/Parser.cpp:257`.

**Cómo funciona:**

1. Reconoce cero o más funciones.
2. Reconoce `main` con su bloque.
3. Si sobran tokens después del bloque de `main`, reporta “contenido
   inesperado”.
4. Devuelve el resultado.

**Ejemplo válido:**

```c
int suma(int a, int b) { return a + b; }
void main() { int x; x = 1; }
```

**Ejemplo inválido:** una función después de `main` produce “contenido
inesperado después del bloque principal”.

---

### 6.2 LISTA_FUNCIONES y FUNCION

**Gramática:**

```bnf
LISTA_FUNCIONES -> FUNCION*
FUNCION         -> TIPO_RETORNO ID ( LISTA_PARAMETROS ) BLOQUE
TIPO_RETORNO    -> void | TIPO_BASE
```

**Código:** `esInicioFuncion()` (`:182`), `reconocerListaFunciones()` (`:283`)
y `reconocerFuncion()` (`:298`).

**Cómo funciona:**

- `esInicioFuncion()` usa **lookahead de un token**:
  - si hay un tipo (`void`, `int`, `float`, ...) y el siguiente es `ID`,
    entonces es una función;
  - si el siguiente es `MAIN`, no es función: es el inicio de `PRINCIPAL`.
- `reconocerFuncion()` consume:
  1. tipo de retorno,
  2. nombre (`ID`),
  3. `(`,
  4. lista de parámetros,
  5. `)`,
  6. bloque.
- Los parámetros son `TIPO ID`, separados por comas; `f(void)` se acepta como
  lista vacía.

**Ejemplo:**

```c
float promedio(float a, float b) {
    return (a + b) / 2;
}
```

**Traza esperada:**

```text
FUNCION
  TIPO_RETORNO: float
  ID: promedio
  PARAMETROS
    TIPO
      TIPO_BASE: float
    PARAMETRO: a (float)
    ...
  BLOQUE
    ...
```

---

### 6.3 PRINCIPAL

**Gramática:**

```bnf
PRINCIPAL -> [TIPO_RETORNO] main ( ) BLOQUE
```

**Código:** `Parser::reconocerPrincipal()` — `src/Parser.cpp:353`.

**Cómo funciona:**

1. Si hay un tipo y el siguiente token es `MAIN`, lo consume como tipo de
   retorno.
2. Consume `main`, `(` y `)`.
3. Acepta `main(void)` como equivalente a `main()`.
4. Llama a `reconocerBloque()`.

Se aceptan `main()`, `void main()` e `int main()` para no rechazar programas
equivalentes.

**Ejemplo mínimo:**

```c
void main() {
    int edad;
    edad = 20;
}
```

---

### 6.4 BLOQUE

**Gramática:**

```bnf
BLOQUE -> { LISTA_VARIABLES COMANDOS }
```

**Código:** `Parser::reconocerBloque()` — `src/Parser.cpp:408`.

**Cómo funciona:**

1. Consume `{`.
2. Llama a `reconocerListaVariables()`: mientras el token actual sea un tipo,
   reconoce declaraciones.
3. Llama a `reconocerComandos()`: reconoce comandos hasta `}`.
4. Consume `}`. Si no aparece, reporta “bloque sin cerrar” e indica la línea
   donde se abrió.

**Regla importante:** las declaraciones van **antes** de los comandos. Si
aparece `int b;` después de una asignación, `reconocerComandos()` reporta:

```text
las variables deben declararse antes de los comandos
```

---

### 6.5 LISTA_VARIABLES, DECLARACION y LISTA_ID

**Gramática:**

```bnf
LISTA_VARIABLES -> (DECLARACION)*
DECLARACION     -> TIPO LISTA_ID ;
LISTA_ID        -> ID DIMENSION* ( , ID DIMENSION* )*
TIPO            -> TIPO_BASE DIMENSION*
DIMENSION       -> [ NUM_INT ]
TIPO_BASE       -> int | float | char | boolean
```

**Código:**

- `reconocerListaVariables()` — `:447`
- `reconocerDeclaracion()` — `:462`
- `reconocerListaVariablesDeclaracion()` — `:479`
- `reconocerTipo()` — `:981`
- `reconocerDimension()` — `:1022`
- `reconocerListaId()` — `:1062`

**Cómo funciona:**

- Mientras el token actual sea `int`, `float`, `char` o `boolean`, hay una
  declaración.
- Se reconoce el tipo, luego uno o más identificadores separados por coma y
  finalmente `;`.
- Cada identificador se registra en la tabla de símbolos con su tipo y
  dimensiones. **No se valida nada**: es solo un registro.
- Las dimensiones se aceptan después del identificador (`int notas[10];`) y
  después del tipo (`int[10] notas;`).

**Ejemplo:**

```c
int edad, contador;
float notas[10], matriz[3][3];
```

---

### 6.6 COMANDOS

**Gramática:**

```bnf
COMANDOS -> COMANDO*
COMANDO  -> ASIGNACION | IF | WHILE | LLAMADA | RETURN
          | SCANF | PRINTLN
```

**Código:** `reconocerComandos()` (`:528`) y `reconocerComando()` (`:552`).

**Cómo funciona:**

`reconocerComando()` mira el token actual y decide:

| Token actual | Producción |
|---|---|
| `ID` | Asignación (o error si parece llamada sin `func`) |
| `IF` | `if/else` |
| `WHILE` | `while` |
| `FUNC` | Llamada `func ...` |
| `SCANF` / `PRINTLN` | Llamada incorporada |
| `RETURN` | `return` |
| `;` | Error: sentencia vacía |
| `{` | Error: bloque inesperado (se recupera) |
| Otro | Token inesperado (se recupera) |

Esta es la función que hace la **selección de alternativas** de la gramática.

---

### 6.7 ASIGNACION

**Gramática:**

```bnf
ASIGNACION -> ID ( [ COMPLEMENTO ] )* = COMPLEMENTO ;
```

**Código:** `reconocerAsignacion()` — `:618`.

**Cómo funciona:**

1. Consume el identificador destino.
2. Si hay corchetes, reconoce subíndices (`notas[i]`, `matriz[1][2]`).
3. Exige `=`.
4. Reconoce `COMPLEMENTO` (la expresión).
5. Exige `;`.

**Ejemplo:**

```c
promedio = (nota1 + nota2) / 2;
notas[i] = notas[i] + 1;
```

Si falta el `=`, el mensaje es:

```text
se esperaba '=' en la asignación de 'edad', pero se encontró '20' (NUM_INT)
```

---

### 6.8 IF / ELSE

**Gramática:**

```bnf
IF -> if ( COMPLEMENTO ) BLOQUE [ else ( IF | BLOQUE ) ]
```

**Código:** `reconocerIf()` — `:671`.

**Cómo funciona:**

1. Consume `if`.
2. Consume `(`, la condición y `)`.
3. Exige un bloque `{ ... }`.
4. Si aparece `else`:
   - si sigue `if`, se reconoce otro `if` (`else if`);
   - si sigue `{`, se reconoce un bloque;
   - si no, error.

**Ejemplo:**

```c
if (edad >= 18) {
    categoria = 1;
} else if (edad >= 65) {
    categoria = 2;
} else {
    categoria = 0;
}
```

**Error típico:** `if (a > 0) a = 2;` → “se esperaba '{' para iniciar el
bloque de 'if'”.

---

### 6.9 WHILE

**Gramática:**

```bnf
WHILE -> while ( COMPLEMENTO ) BLOQUE
```

**Código:** `reconocerWhile()` — `:726`.

**Cómo funciona:**

1. Consume `while`.
2. Consume `(` condición `)`.
3. Exige bloque.
4. La condición se analiza con la misma jerarquía de expresiones que una
   asignación.

**Ejemplo:**

```c
while (contador < 10) {
    suma = suma + contador;
    contador = contador + 1;
}
```

---

### 6.10 LLAMADA y el token `func`

**Gramática:**

```bnf
LLAMADA -> func ID ( LISTA_ARGUMENTOS ) [ ; ]
```

**Código:** `reconocerLlamada()` — `:769`.

**Cómo funciona:**

1. Consume `FUNC` (la palabra reservada `func`).
2. Consume el nombre (`ID`).
3. Consume `(`, los argumentos separados por coma y `)`.
4. Si la llamada es un comando, exige `;`.
5. Si la llamada está dentro de una expresión, no consume `;`.

**Ejemplo como comando:**

```c
func saludar();
```

**Ejemplo dentro de una expresión:**

```c
x = func suma(4, 6);
println(func duplicar(valor));
```

**¿Por qué `func`?** La especificación indica que la producción de llamada usa
ese terminal; por eso se agregó como palabra reservada al lexer
(`src/Lexer.cpp:21`) y como `FUNC` en `Token.h`.

---

### 6.11 RETURN, SCANF y PRINTLN

**Código:** `reconocerReturn()` (`:872`) y `reconocerLlamadaIncorporada()`
(`:827`).

- `return` acepta una expresión opcional y exige `;`.
- `scanf(...)` y `println(...)` se reconocen como comandos incorporados con
  lista de argumentos y `;`.

**Ejemplos:**

```c
return resultado;
return;
scanf(edad);
println("hola");
```

---

### 6.12 Expresiones y precedencia

**Gramática:**

```bnf
COMPLEMENTO -> OR
OR          -> AND ( || AND )*
AND         -> REL ( && REL )*
REL         -> ADD ( COMP ADD )*
ADD         -> MUL ( (+|-) MUL )*
MUL         -> UNARIO ( (*|/|%) UNARIO )*
UNARIO      -> (-|!) UNARIO | PRIMARIO
PRIMARIO    -> NUM_INT | NUM_DEC | TEXTO
             | func ID ( LISTA_ARGUMENTOS )
             | ID ( [ COMPLEMENTO ] )*
             | ( COMPLEMENTO )
```

**Código:** cadena de funciones en `src/Parser.cpp:1133-1288`.

**Cómo se lee la precedencia:**

De menor a mayor prioridad:

```text
||   ->   &&   ->   relacionales   ->   + -   ->   * / %   ->   unarios   ->   primarios
```

Ejemplo:

```c
r = a + b * c < 10 && !x || y;
```

Se agrupa así:

```text
( ( (a + (b * c)) < 10 ) && (!x) ) || y
```

- `b * c` se resuelve antes que `a + ...`.
- La suma se resuelve antes que `<`.
- `<` se resuelve antes que `&&`.
- `&&` se resuelve antes que `||`.

Cada nivel tiene su función (`reconocerOr`, `reconocerAnd`,
`reconocerRelacional`, `reconocerAditivo`, `reconocerMultiplicativo`,
`reconocerUnario`, `reconocerPrimario`). Esa es la forma clásica de implementar
precedencia en descenso recursivo sin ambigüedad.

---

## 7. Errores y recuperación

### 7.1 Cómo se reporta un error

```text
[ERROR SINTACTICO] Línea 3: se esperaba ';' al final de la declaración
(LISTA_VARIABLES), pero se encontró 'edad' (ID)
      3 |     edad = 20;
        |     ^
        Sugerencia: Falta ';' al final de la sentencia de la línea 2.
```

Incluye:

- tipo de error (léxico o sintáctico),
- línea,
- mensaje con lo esperado y lo encontrado,
- fragmento de código,
- caret que apunta a la columna exacta (la columna no se imprime, pero se usa
  para ubicar el `^`),
- sugerencia.

### 7.2 Recuperación

`Parser::sincronizar()` (`:176`) avanza hasta `;` o `}`. Eso permite reportar
varios errores en una sola ejecución en lugar de detenerse en el primero.

Ejemplo: si falta un `;` antes de una asignación, el parser reporta el error y
**no consume** el identificador de la siguiente sentencia; así puede seguir
reconociendo la asignación y encontrar más problemas.

### 7.3 Errores típicos que debe detectar

| Error | Ejemplo | Mensaje |
|---|---|---|
| Falta `;` | `int edad` | `se esperaba ';' ...` |
| Falta `}` | bloque sin cierre | `bloque sin cerrar ...` |
| Falta `)` | `if (a > 0 {` | `se esperaba ')' ...` |
| Falta `]` | `int notas[10;` | `se esperaba ']' ...` |
| Token inesperado | `123;` | `token inesperado ...` |
| Declaración después de comando | `a = 1; int b;` | `las variables deben declararse antes de los comandos` |
| Falta `=` | `edad 20;` | `se esperaba '=' ...` |
| Expresión incompleta | `a = ;` | `se esperaba una expresión ...` |
| Función sin `(` | `int suma {` | `'(' después del nombre de la función` |
| Llamada sin `func` | `saludar();` | `las llamadas a función deben comenzar con 'func'` |
| `else` sin `if` | `else { ... }` | `token inesperado 'else' ...` |
| Carácter inválido | `@` | `carácter no reconocido '@'` |

---

## 8. Sintaxis vs semántica

La especificación dice:

> “Mantener separado el análisis sintáctico del análisis semántico. En este
> proyecto no se exige comprobar declaración de variables, compatibilidad de
> tipos ni ejecución del programa.”

Por eso:

- **Sí** se registra una variable declarada en la tabla de símbolos.
- **No** se comprueba si una variable fue declarada antes de usarla.
- **No** se comprueba si los tipos coinciden.
- **No** se comprueba si una función existe.
- **No** se ejecuta el programa.

Si la profesora pregunta “¿por qué no valida que `x` exista?”, la respuesta es:

> “Porque eso es análisis semántico y la especificación lo separa. El parser
> solo decide si la estructura es sintácticamente correcta.”

---

## 9. Pruebas y evidencias

### 9.1 Cómo se ejecutan

```bash
./pruebas/ejecutar_pruebas.sh
```

El script:

1. Compila si falta `build/analizador` (o si se usa `--recompilar`).
2. Ejecuta todos los `.lp` de `casos_de_prueba/`.
3. Exige código `0` para los `valido_*.lp` y `1` para los `invalido_*.lp`.
4. Verifica también el veredicto impreso.
5. Guarda la salida en `evidencias/<nombre>.txt`.
6. Genera `evidencias/resumen.txt`.

### 9.2 Qué cubren

- **12 válidos:** mínimo, completo, funciones, `if/else`, `while`, anidados,
  expresiones, llamadas, arreglos, sin variables, `scanf/println` y `f(void)`.
- **17 inválidos:** puntuación, llaves, paréntesis, corchetes, declaraciones
  fuera de orden, funciones, llamadas, expresiones y `else` sin `if`.

Resultado actual:

```text
Total de casos:  29
Aprobados:       29
Fallidos:        0
```

### 9.3 Qué evidencia mostrar

- `evidencias/valido_funciones.txt`: funciones, parámetros, `return`, llamadas.
- `evidencias/valido_if_else.txt`: `if/else`, `else if`, precedencia.
- `evidencias/invalido_falta_punto_coma.txt`: error clásico.
- `evidencias/invalido_declaracion_despues_comando.txt`: regla del bloque.
- `evidencias/resumen.txt`: los 29 casos aprobados.

---

## 10. Guion de exposición (8–10 minutos)

### Integrante A – Lexer e integración (4 min)

1. Mostrar `src/Lexer.cpp`: cómo se forman los tokens.
2. Mostrar `src/Token.h`: `ID`, `NUM_INT`, `NUM_DEC`, `TEXTO`, `COMP`, `FUNC`.
3. Mostrar el constructor de `Parser`: cómo llama a `getNextToken()` y llena
   `tokens`/`flujo`.
4. Mostrar la tabla de tokens en una ejecución.
5. Explicar que el parser no lee caracteres.

### Integrante B – Parser y gramática (5 min)

1. Mostrar `reconocerPrograma`: `LISTA_FUNCIONES` + `PRINCIPAL`.
2. Mostrar `reconocerFuncion`: tipo, nombre, parámetros, bloque.
3. Mostrar `reconocerBloque`: `LISTA_VARIABLES` y después `COMANDOS`.
4. Mostrar `reconocerComando`: asignación, `if`, `while`, `func`, `return`.
5. Mostrar la cadena de expresiones y explicar precedencia con un ejemplo.
6. Mostrar un error y la recuperación con `sincronizar()`.

### Cierre (1 min)

- Ejecutar la suite: 29/29.
- Mostrar `evidencias/resumen.txt`.
- Recordar: sintaxis separada de semántica y sin `for`.

---

## 11. Preguntas probables de la profesora

**¿Dónde se integra el lexer con el parser?**
En el constructor de `Parser` (`src/Parser.cpp:13`): crea un `Lexer`, llama a
`getNextToken()` en un ciclo y guarda los tokens. El parser nunca lee
caracteres para analizar.

**¿Por qué hay dos vectores, `tokens` y `flujo`?**
`tokens` conserva todo para mostrar la tabla; `flujo` es la secuencia que
consume el parser, sin tokens `DESCONOCIDO` y con `FIN_ARCHIVO` al final.

**¿Cómo se distingue una función de `main`?**
Con `esInicioFuncion()`: tipo + `ID` es función; tipo + `MAIN` es `main`.

**¿Cómo se obliga a declarar antes de los comandos?**
`reconocerBloque` llama primero a `reconocerListaVariables` y luego a
`reconocerComandos`. Si aparece un tipo en la fase de comandos, se reporta el
error.

**¿Cómo se detecta un `;` faltante?**
Se llama a `consumir(PUNTO_COMA)`. Si devuelve `false`, `errorPuntoComa`
reporta el token encontrado y sugiere la línea donde probablemente falta.

**¿Cómo se recupera de un error?**
`sincronizar()` avanza hasta `;` o `}`. Si el token actual inicia otra
sentencia, no se consume para que el bloque la procese.

**¿Cómo se implementa la precedencia?**
Con una función por nivel: `OR -> AND -> REL -> ADD -> MUL -> UNARIO ->
PRIMARIO`. Cada una llama a la siguiente y repite su operador.

**¿Dónde está `func`?**
En `Token.h` como `FUNC`, en `Lexer.cpp` como palabra reservada y en
`reconocerLlamada`/`reconocerPrimario` como terminal de llamada.

**¿Se valida que una variable exista?**
No. Es análisis semántico y la especificación pide separarlo.

**¿Se genera un AST?**
La salida incluye la traza/árbol de reconocimiento y el resultado. No se
construye un AST independiente; la estructura ya reconoce cada producción y
podría servir de base para uno.

**¿Por qué no hay `for`?**
Porque la gramática de la especificación no incluye esa producción.

**¿Qué pasa si el programa es inválido?**
Se reportan uno o más errores, se sigue analizando cuando es posible y el
proceso termina con código `1`.

**¿Cómo se prueba?**
Con `./pruebas/ejecutar_pruebas.sh`: 29 casos, comparación de código de salida
y veredicto, y evidencias en `evidencias/`.

---

## 12. Limitaciones (y cómo responder)

| Limitación | Respuesta |
|---|---|
| No se reconoce `for` | La especificación no lo incluye; se reporta como token inesperado |
| No hay AST independiente | La traza muestra el árbol de reconocimiento; un AST es un siguiente paso, no un requisito técnico explícito |
| No hay análisis semántico | Es una decisión de la especificación: sintaxis separada de semántica |
| `if/while` exigen bloque | La gramática define `BLOQUE`, no un comando suelto |
| Arreglos de tamaño constante | `DIMENSION -> [ NUM_INT ]` |
| Sin inicialización en declaración | `LISTA_VARIABLES` no incluye `= expresión`; se declara y luego se asigna |
| Comentarios `//` y textos con `"..."` | Es lo que soporta el lexer actual |

---

## 13. Comandos para la demo

```bash
# Compilar
cmake -S . -B build && cmake --build build -j

# 1. Programa mínimo
./build/analizador casos_de_prueba/valido_basico.lp

# 2. Funciones y llamadas
./build/analizador casos_de_prueba/valido_funciones.lp --sin-tokens

# 3. if/else y while
./build/analizador casos_de_prueba/valido_if_else.lp --sin-tokens

# 4. Error de punto y coma
./build/analizador casos_de_prueba/invalido_falta_punto_coma.lp \
    --sin-tokens --sin-traza --sin-tabla

# 5. Declaración después de comando
./build/analizador casos_de_prueba/invalido_declaracion_despues_comando.lp \
    --sin-tokens --sin-traza --sin-tabla

# 6. Suite completa
./pruebas/ejecutar_pruebas.sh
```

---

## Apéndice A – Gramática completa

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

## Apéndice B – Tokens

| Terminal | Token |
|---|---|
| `id` | `ID` |
| `num_int` | `NUM_INT` |
| `num_dec` | `NUM_DEC` |
| `texto` | `TEXTO` |
| `comp` | `COMP` |
| `func` | `FUNC` |
| Tipos | `INT`, `FLOAT`, `CHAR`, `BOOLEAN`, `VOID` |
| Control | `MAIN`, `IF`, `ELSE`, `WHILE`, `RETURN` |
| Entrada/salida | `SCANF`, `PRINTLN` |

## Apéndice C – Glosario

- **Token:** unidad léxica producida por el lexer (`ID`, `NUM_INT`, `;`, ...).
- **No terminal:** símbolo de la gramática que se expande en una producción
  (`PROGRAMA`, `BLOQUE`, `COMANDO`, ...).
- **Descenso recursivo:** técnica de parsing donde cada no terminal tiene una
  función que consume tokens y llama a las funciones de sus no terminales.
- **Lookahead:** mirar el siguiente token sin consumirlo.
- **Recuperación:** seguir analizando después de un error para reportar más de
  uno.
- **Sintaxis:** forma válida de las construcciones.
- **Semántica:** significado, tipos y validez de las operaciones.
