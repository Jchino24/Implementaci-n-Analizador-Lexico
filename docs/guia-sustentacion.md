# Guía de sustentación – Analizador sintáctico LP

Esta guía está alineada con `Gramatica_LP_LL1.docx`. Resume qué pide la
profesora, cómo funciona el parser y qué responder.

---

## 1. Qué pide la profesora

Un analizador sintáctico por **descenso recursivo** que:

1. Reutilice el lexer y consuma su secuencia de tokens.
2. Reconozca la gramática LL(1) de LP:
   - `PROGRAMA -> LISTA_FUNCIONES PRINCIPAL`;
   - funciones antes de `main`;
   - `main()` seguido de bloque;
   - bloque con declaraciones antes de comandos;
   - `if/else` y `while`;
   - llamadas con `func`;
   - expresiones con precedencia.
3. No implemente `for`.
4. No haga análisis semántico.
5. Distinga aceptación de error.

En una frase:

> “El lexer produce tokens; el parser los consume con una función por no
> terminal y decide si forman un programa LP válido. Si no, reporta el error.”

---

## 2. Flujo

```text
main.cpp
  -> Parser(ruta)
     -> Lexer::getNextToken()  (tokens)
     -> Parser::analizar()
        -> reconocerPrograma()
           -> reconocerListaFunciones()
              -> reconocerDeclaracionFuncion()
           -> reconocerPrincipal()
              -> reconocerBloque()
                 -> reconocerListaVariables()
                 -> reconocerComandos()
                    -> LECTURA | ESCRITURA | ASIGNACION | FUNCION
                     | SELECCION | MIENTRAS | RETORNO
```

`Parser` guarda `tokens` (para la tabla) y `flujo` (secuencia que consume).
`actual()`, `siguiente()`, `avanzar()`, `comprobar()` y `consumir()` son el
motor del descenso recursivo.

---

## 3. Reglas que hay que recordar

| Regla | Ejemplo correcto | Ejemplo incorrecto |
|---|---|---|
| `main` con o sin `void` | `main() { ... }` / `void main() { ... }` | — |
| Dimensiones antes del id | `int[10] notas;` | `int notas[10];` |
| `func` sin `;` como comando | `func saludar()` | `func saludar();` |
| `func` con `;` solo como complemento | `x = func f();` | — |
| `return` con expresión | `return x;` | `return;` |
| `scanf` solo variables | `scanf(x);` | `scanf(1);` |
| `println` solo id/const/texto | `println(x, 1, "hola");` | `println(x + 1);` |
| Sin asignar a arreglo | `x = notas[0];` | `notas[0] = 1;` |
| Sin `else if` | `else { if (...) { ... } }` | `else if (...) { ... }` |
| Sin `for` | `while` | `for` |

---

## 4. Dónde está cada cosa

| Producción | Función |
|---|---|
| `PROGRAMA` | `reconocerPrograma` |
| `LISTA_FUNCIONES` | `reconocerListaFunciones` |
| `DECLARACION_FUNCION` | `reconocerDeclaracionFuncion` |
| `TIPO_RETORNO` | `reconocerTipoRetorno` |
| `TIPO` / `TIPO_BASE` / `DIMENSION` | `reconocerTipo`, `reconocerTipoBase`, `reconocerDimension` |
| `PARAMETROS` | `reconocerParametros` |
| `PRINCIPAL` | `reconocerPrincipal` |
| `BLOQUE` | `reconocerBloque` |
| `LISTA_VARIABLES` / `LISTA_ID` | `reconocerListaVariables`, `reconocerListaId` |
| `COMANDOS` / `COMANDO` | `reconocerComandos`, `reconocerComando` |
| `LECTURA` / `ESCRITURA` | `reconocerLectura`, `reconocerEscritura` |
| `SELECCION` / `SINO` | `reconocerSeleccion`, `reconocerSino` |
| `MIENTRAS` | `reconocerMientras` |
| `ASIGNACION` / `COMPLEMENTO` | `reconocerAsignacion`, `reconocerComplemento` |
| `FUNCION` / `ARGUMENTOS` | `reconocerFuncion`, `reconocerArgumentos` |
| `RETORNO` | `reconocerRetorno` |
| `EXPRESION` y niveles | `reconocerExpresionO`, `Y`, `Relacional`, `Aditiva`, `Multiplicativa`, `Factor` |

---

## 5. Precedencia de expresiones

De menor a mayor prioridad:

```text
||  ->  &&  ->  relacionales  ->  + -  ->  * / %  ->  unarios/signo  ->  término
```

Ejemplo:

```c
r = a + b * c < 10 && !x || y;
```

Se agrupa como:

```text
(((a + (b * c)) < 10) && (!x)) || y
```

Funciones: `reconocerExpresionO`, `reconocerExpresionY`,
`reconocerExpresionRelacional`, `reconocerExpresionAditiva`,
`reconocerExpresionMultiplicativa`, `reconocerFactor`, `reconocerTermino`.

---

## 6. Errores y recuperación

- `errorPuntoComa` reporta `;` faltante y sugiere la línea.
- `errorActual` reporta “se esperaba X, se encontró Y”.
- `sincronizar()` avanza hasta `;` o `}` para seguir analizando.
- `analizar()` devuelve `true` solo si no hay errores; el proceso sale con `0`,
  `1` o `2`.

Ejemplo:

```text
[ERROR SINTACTICO] Línea 3: se esperaba ';' al final de la declaración
(LISTA_VARIABLES), pero se encontró 'edad' (ID)
      3 |     edad = 20;
        |     ^
        Sugerencia: Falta ';' al final de la sentencia de la línea 2.
```

---

## 7. Sintaxis vs semántica

No se comprueba si una variable fue declarada, si los tipos coinciden ni si una
función existe. La tabla de símbolos solo registra declaraciones. Eso es
análisis semántico y la especificación pide mantenerlo separado.

---

## 8. Preguntas probables

**¿Dónde se integra el lexer?**
En el constructor de `Parser`: crea un `Lexer`, llama a `getNextToken()` y
llena `tokens`/`flujo`.

**¿Por qué se acepta `void main()`?**
La gramática define `PRINCIPAL -> main ( ) BLOQUE`, pero el ejemplo del curso
usa `void main()`. Se acepta `void` opcional antes de `main` por compatibilidad.

**¿Por qué las dimensiones van antes del id?**
Porque `TIPO -> TIPO_BASE DIMENSION`; el tipo completo incluye los corchetes:
`int[10] notas;`.

**¿Por qué `func` no lleva `;`?**
Porque `FUNCION -> func id ( ARGUMENTOS )`. El `;` lo aporta `ASIGNACION`
cuando la llamada es el complemento: `x = func f();`.

**¿Cómo distingue función de `main`?**
`esInicioFuncion()` mira si hay un tipo y luego un `ID`; si el siguiente es
`MAIN`, es `main`.

**¿Cómo obliga a declarar antes de los comandos?**
`BLOQUE` llama primero a `reconocerListaVariables` y después a
`reconocerComandos`. Si aparece un tipo en la fase de comandos, reporta error.

**¿Cómo detecta un `;` faltante?**
Con `consumir(PUNTO_COMA)`. Si falla, `errorPuntoComa` reporta el token
encontrado.

**¿Cómo evita bucles infinitos?**
Cada función consume al menos un token o llama a `sincronizar()`, que avanza
hasta `;` o `}`.

**¿Por qué no hay `else if`?**
Porque `SINO -> else BLOQUE | ε`. Para anidar se usa `else { if ... }`.

**¿Por qué no se valida el uso de variables?**
Es análisis semántico; la especificación lo separa de la sintaxis.

**¿Se genera un AST?**
Se muestra la traza/árbol de reconocimiento y el resultado; no se construye un
AST independiente.

---

## 9. Demostración

```bash
cmake -S . -B build && cmake --build build -j

# Válido
./build/analizador casos_de_prueba/valido_funciones.lp --sin-tokens

# if/else y while
./build/analizador casos_de_prueba/valido_if_else.lp --sin-tokens

# Error clásico
./build/analizador casos_de_prueba/invalido_falta_punto_coma.lp \
    --sin-tokens --sin-traza --sin-tabla

# Suite: 34 casos
./pruebas/ejecutar_pruebas.sh
```

Mostrar en este orden:

1. Tabla de tokens (integración con el lexer).
2. Traza con `DECLARACION_FUNCION`, `PRINCIPAL`, `BLOQUE`, `SELECCION`,
   `MIENTRAS`, `FUNCION`.
3. Un error con línea, fragmento y sugerencia.
4. `evidencias/resumen.txt`: 34/34.
