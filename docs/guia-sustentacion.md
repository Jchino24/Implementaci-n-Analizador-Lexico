# Guía breve de sustentación – Proyecto integrador LP

Esta guía resume el flujo del sistema y las respuestas esperadas para la
sustentación. No sustituye la lectura del código: sirve como mapa.

---

## 1. Flujo completo de una ejecución

```text
main.cpp
  ├─ lee argumentos y resuelve la ruta del .lp
  ├─ crea Parser(ruta)
  │    ├─ lee el archivo línea por línea (para mostrar fragmentos con ^)
  │    ├─ crea Lexer(ruta)
  │    ├─ llama Lexer::getNextToken() hasta FIN_ARCHIVO
  │    ├─ guarda todos los tokens (tabla de tokens)
  │    └─ guarda los tokens útiles en `flujo` (sin DESCONOCIDO)
  ├─ Parser::analizar()
  │    └─ reconocerPrograma()
  │         ├─ reconocerListaFunciones()
  │         │    └─ reconocerFuncion()
  │         │         ├─ TIPO_RETORNO
  │         │         ├─ ID
  │         │         ├─ PARAMETROS
  │         │         └─ BLOQUE
  │         └─ reconocerPrincipal()
  │              ├─ [TIPO_RETORNO] main ( )
  │              └─ BLOQUE
  │                   ├─ LISTA_VARIABLES
  │                   │    └─ DECLARACION -> TIPO LISTA_ID ;
  │                   └─ COMANDOS
  │                        ├─ ASIGNACION
  │                        ├─ IF / IF-ELSE
  │                        ├─ WHILE
  │                        ├─ LLAMADA (func ...)
  │                        ├─ RETURN
  │                        └─ SCANF / PRINTLN
  └─ imprime tokens, traza, tabla de símbolos, diagnóstico y resultado
```

El método `analizar()` devuelve `true` solo si no hubo errores. `main.cpp`
traduce eso a los códigos de salida `0` (válido) y `1` (con errores);
`2` se reserva para errores de uso o de archivo.

---

## 2. Puntos clave del código

| Archivo | Responsabilidad |
|---|---|
| `src/Lexer.cpp` | Convierte caracteres en tokens; reconoce `func`, comentarios `//`, números, textos, operadores y palabras reservadas. Registra errores léxicos. |
| `src/Token.h` | Define `TokenType` (incluido `FUNC`) y `Token` (tipo, lexema, línea, columna). |
| `src/Parser.cpp` | Una función por no terminal. `reconocerPrograma`, `reconocerFuncion`, `reconocerPrincipal`, `reconocerBloque`, `reconocerComandos`, `reconocerIf`, `reconocerWhile`, `reconocerLlamada`, etc. |
| `src/Parser.h` | Declara la interfaz, la traza, los errores y `InfoTipo`. |
| `src/TablaSimbolos.h` | Registra nombre, tipo (con dimensiones), línea y columna. No valida semántica. |
| `src/main.cpp` | CLI, formato de salida, fragmentos de código con `^` y códigos de salida. |

Conceptos usados:

- **Análisis recursivo descendente:** cada no terminal tiene una función.
- **Token actual (`actual()`) y avance (`avanzar()`):** el parser mira un
  token a la vez, sin retroceder.
- **Lookahead de un token (`siguiente()`):** permite distinguir una función
  (`tipo` + `ID`) de `main` (`tipo` + `MAIN`), y una llamada de una asignación.
- **Traza:** `abrirNodo` / `nodo` / `cerrarNodo` registran las producciones
  reconocidas; `CierreNodo` cierra el nodo automáticamente al salir.
- **Recuperación de errores:** `sincronizar()` avanza hasta `;` o `}` para
  seguir analizando y reportar más de un error.

---

## 3. Decisiones de diseño

1. **El parser no lee caracteres.** Solo consume los tokens que produce el
   `Lexer`; la integración está en el constructor de `Parser`.
2. **Declaraciones antes de comandos.** `BLOQUE` reconoce primero
   `LISTA_VARIABLES` y después `COMANDOS`. Si aparece una declaración después
   de un comando, se reporta un error específico.
3. **Sintaxis separada de semántica.** No se comprueba si una variable fue
   declarada, si los tipos coinciden ni si una función existe. La tabla de
   símbolos es solo un registro.
4. **`func` es una palabra reservada** para llamadas: `func suma(2, 3);` o
   `x = func suma(2, 3);`.
5. **Precedencia de expresiones:** `||` < `&&` < relacionales < aditivos <
   multiplicativos < unarios < primarios.
6. **`for` no se implementa** porque la gramática de la especificación no lo
   incluye. `if/else` y `while` sí.

---

## 4. Reparto sugerido para la exposición

**Integrante A – Lexer e integración**

1. Cómo `Parser` crea el `Lexer` y llena la secuencia de tokens.
2. Cómo se reconocen identificadores, números, textos y `func`.
3. Qué pasa con un carácter desconocido (`@`) y cómo se reporta.
4. Cómo se llena la tabla de símbolos en `reconocerListaId`.

**Integrante B – Parser y errores**

1. Cómo `reconocerPrograma` separa `LISTA_FUNCIONES` de `PRINCIPAL`.
2. Cómo se reconoce una función y cómo se distingue de `main`.
3. Cómo `BLOQUE` obliga a declarar antes de los comandos.
4. Cómo se reconocen `if/else`, `while` y las llamadas con `func`.
5. Cómo se aplica la precedencia de operadores.
6. Cómo se detecta y recupera el error de punto y coma faltante.
7. Cómo se ejecutan las pruebas y qué significan los códigos de salida.

---

## 5. Preguntas frecuentes

**¿Dónde se integra el lexer con el parser?**
En el constructor de `Parser`: se crea un `Lexer` con la ruta, se llama a
`getNextToken()` en un ciclo y se guardan los tokens. El parser nunca lee
caracteres directamente.

**¿Qué diferencia hay entre `tokens` y `flujo`?**
`tokens` conserva todo para la tabla de tokens (incluidos los `DESCONOCIDO`).
`flujo` es la secuencia que consume el parser: excluye los tokens
desconocidos (ya reportados como error léxico) y termina en `FIN_ARCHIVO`.

**¿Cómo se distingue una función de `main`?**
`esInicioFuncion()` comprueba que el token actual sea un tipo (`void`, `int`,
...) y que el siguiente sea `ID`. Si el siguiente es `MAIN`, la función
termina y comienza `PRINCIPAL`.

**¿Cómo se obliga a declarar antes de los comandos?**
`reconocerBloque` llama primero a `reconocerListaVariables` (mientras haya un
tipo al inicio) y luego a `reconocerComandos`. Si en la fase de comandos
aparece un tipo, se reporta “las variables deben declararse antes de los
comandos”.

**¿Cómo se detecta que falta un `;`?**
Después de `LISTA_ID`, de `COMPLEMENTO`, de `RETURN` o de una llamada, el
parser llama a `consumir(PUNTO_COMA)`. Si devuelve `false`, `errorPuntoComa`
reporta el token encontrado y sugiere la línea donde probablemente falta.

**¿Cómo evita un bucle infinito al recuperarse?**
Cada rama de `COMANDOS` consume al menos un token o llama a `sincronizar()`,
que avanza hasta `;` o `}`. Si el token actual inicia otra sentencia, no se
consume para que el bloque la procese.

**¿Cómo se reconocen `if/else` y `while`?**
`reconocerIf` consume `if`, la condición entre paréntesis y un `BLOQUE`;
luego, si aparece `else`, acepta otro bloque o un `if` (para `else if`).
`reconocerWhile` consume `while`, la condición y un `BLOQUE`.

**¿Cómo se manejan las expresiones?**
Con seis niveles: `OR`, `AND`, `REL`, `ADD`, `MUL`, `UNARIO` y `PRIMARIO`.
Así se respeta la precedencia indicada por la especificación. `PRIMARIO`
resuelve literales, identificadores, subíndices, llamadas `func` y paréntesis.

**¿Se comprueba si una variable existe o si los tipos coinciden?**
No. La especificación pide separar la sintaxis de la semántica. Esas
validaciones quedan fuera del alcance y no afectan al resultado del parser.

**¿Se genera un AST?**
La salida incluye la traza/árbol de reconocimiento con las producciones
reconocidas y el resultado del análisis. No se construye un AST independiente
en esta entrega.

**¿Qué pasa si el programa usa `for`?**
Se reporta como token inesperado, porque la gramática de la especificación no
incluye esa producción.

**¿Cómo se prueba el sistema?**
`./pruebas/ejecutar_pruebas.sh` compila, ejecuta los 29 casos, compara el
código de salida y el veredicto impreso, y guarda la salida completa en
`evidencias/`.

---

## 6. Demostración sugerida (5 minutos)

```bash
# 1. Compilar
cmake -S . -B build && cmake --build build -j

# 2. Programa mínimo
./build/analizador casos_de_prueba/valido_basico.lp

# 3. Funciones, llamadas y return
./build/analizador casos_de_prueba/valido_funciones.lp --sin-tokens

# 4. if/else y while
./build/analizador casos_de_prueba/valido_if_else.lp --sin-tokens

# 5. Error: falta ';' en una declaración
./build/analizador casos_de_prueba/invalido_falta_punto_coma.lp \
    --sin-tokens --sin-traza --sin-tabla

# 6. Error: declaración después de un comando
./build/analizador casos_de_prueba/invalido_declaracion_despues_comando.lp \
    --sin-tokens --sin-traza --sin-tabla

# 7. Suite completa
./pruebas/ejecutar_pruebas.sh
```

Durante la demo conviene señalar:

- La tabla de tokens demuestra la integración con el lexer.
- La traza muestra cada producción: `PROGRAMA`, `LISTA_FUNCIONES`, `FUNCION`,
  `PRINCIPAL`, `BLOQUE`, `LISTA_VARIABLES`, `COMANDOS`, `IF`, `WHILE`,
  `LLAMADA`, `RETURN`.
- El diagnóstico incluye línea, fragmento y sugerencia; la columna se usa
  internamente para ubicar el caret.
- El código de salida permite usar el analizador en scripts de prueba.
