# Guía breve de sustentación – Entregable 1

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
  │         ├─ reconocerTipoBase(true)   -> void / int / float / char / boolean
  │         ├─ reconocerPrincipal()      -> main ( )
  │         └─ reconocerBloque()         -> { ... }
  │              ├─ reconocerDeclaracion()
  │              │    ├─ reconocerTipo() -> TIPO_BASE DIMENSION*
  │              │    └─ reconocerListaVariables()
  │              │         ├─ reconocerListaId()  (inserta en la tabla)
  │              │         └─ consume ';'
  │              └─ reconocerAsignacion()
  │                   ├─ ID y subíndices opcionales
  │                   ├─ consume '='
  │                   ├─ reconocerComplemento()  (expresiones)
  │                   └─ consume ';'
  └─ imprime tokens, traza, tabla de símbolos, diagnóstico y resultado
```

El método `analizar()` devuelve `true` solo si no hubo errores. `main.cpp`
traduce eso a los códigos de salida `0` (válido) y `1` (con errores).

---

## 2. Puntos clave del código

| Archivo | Responsabilidad |
|---|---|
| `src/Lexer.cpp` | Convierte caracteres en tokens; reconoce comentarios `//`, números, textos, operadores, palabras reservadas y símbolos. Registra errores léxicos. |
| `src/Token.h` | Define `TokenType` y `Token` (tipo, lexema, línea, columna). |
| `src/Parser.cpp` | Una función por no terminal (`reconocerPrograma`, `reconocerBloque`, etc.). Consume tokens con `consumir`, comprueba con `comprobar` y reporta con `errorActual` / `errorPuntoComa`. |
| `src/Parser.h` | Declara la interfaz, la traza, los errores y `InfoTipo`. |
| `src/TablaSimbolos.h` | Guarda nombre, tipo (incluye dimensiones), línea y columna. `insertar` detecta duplicados. |
| `src/main.cpp` | CLI, formato de salida, fragmentos de código con `^` y códigos de salida. |

Conceptos usados:

- **Análisis recursivo descendente:** cada no terminal tiene una función.
- **Token actual (`actual()`) y avance (`avanzar()`):** el parser mira un
  token a la vez, sin retroceder.
- **Traza:** `abrirNodo` / `nodo` / `cerrarNodo` registran las producciones
  reconocidas; `CierreNodo` cierra el nodo automáticamente al salir de la
  función (incluso si hay un `return` anticipado).
- **Recuperación de errores:** `sincronizar()` avanza hasta `;` o `}` para
  seguir analizando y reportar más de un error.

---

## 3. Reparto sugerido para la exposición

Ambos integrantes deben poder explicar todo el flujo; este reparto permite
dividir la presentación sin perder continuidad:

**Integrante A – Lexer e integración**

1. Cómo `Parser` crea el `Lexer` y llena la secuencia de tokens.
2. Cómo se reconocen identificadores, números, textos y palabras reservadas.
3. Qué pasa con un carácter desconocido (`@`) y cómo se reporta.
4. Cómo se llena la tabla de símbolos en `reconocerListaId`.

**Integrante B – Parser y errores**

1. Cómo `reconocerPrograma` verifica la cabecera y llama a `PRINCIPAL` y
   `BLOQUE`.
2. Cómo se distingue una declaración de una asignación (primer token:
   `TIPO_BASE` vs. `ID`).
3. Cómo se reconoce `COMPLEMENTO` con precedencia de operadores.
4. Cómo se detecta y recupera el error de punto y coma faltante.
5. Cómo se ejecutan las pruebas y qué significan los códigos de salida.

---

## 4. Preguntas frecuentes

**¿Dónde se integra el lexer con el parser?**
En el constructor de `Parser`: se crea un `Lexer` con la ruta, se llama a
`getNextToken()` en un ciclo y se guardan los tokens. El parser nunca lee
caracteres directamente.

**¿Qué diferencia hay entre `tokens` y `flujo`?**
`tokens` conserva todo para la tabla de tokens (incluidos los `DESCONOCIDO`).
`flujo` es la secuencia que consume el parser: excluye los tokens
desconocidos (ya reportados como error léxico) y termina en `FIN_ARCHIVO`.

**¿Por qué se usa `FIN_ARCHIVO`?**
Para que todas las funciones tengan un token actual válido y para detectar
errores como “bloque sin cerrar” sin leer fuera del vector.

**¿Cómo se detecta que falta un `;`?**
Después de `LISTA_ID` o de `COMPLEMENTO`, el parser llama a
`consumir(PUNTO_COMA)`. Si devuelve `false`, `errorPuntoComa` reporta el token
encontrado y sugiere la línea donde probablemente falta el `;`.

**¿Cómo evita un bucle infinito al recuperarse?**
Cada rama del bloque consume al menos un token o llama a `sincronizar()`, que
avanza hasta `;` o `}`. Si el token actual inicia otra sentencia
(`TIPO_BASE` o `ID`), no se consume para que el bloque la procese.

**¿Cómo se reconoce `int notas[10];` si la gramática dice
`TIPO -> TIPO_BASE DIMENSION*`?**
Se admiten dos formas: dimensiones después del tipo (`int[10] notas;`) y
después del identificador (`int notas[10];`). En `reconocerListaId` se llama a
`reconocerDimension` mientras el token actual sea `[`.

**¿Cómo se manejan las expresiones?**
Con tres niveles: `COMPLEMENTO` (comparaciones), `EXPRESION` (suma, resta,
`||`) y `TERMINO` (multiplicación, división, módulo, `&&`). `FACTOR` resuelve
paréntesis, signos unarios, literales, identificadores y subíndices.

**¿Qué pasa si una variable se usa sin declarar?**
Se registra una **advertencia** (no un error sintáctico), porque la
verificación de tipos y declaraciones es semántica y queda fuera del alcance
del Entregable 1. El programa puede seguir siendo válido sintácticamente.

**¿Cómo se prueba el sistema?**
`./pruebas/ejecutar_pruebas.sh` compila, ejecuta los 9 casos, compara el
código de salida esperado y guarda la salida completa en `evidencias/`.

**¿Qué limitaciones tiene esta entrega?**
No hay bloques anidados, estructuras de control, funciones adicionales ni
inicialización en la declaración. La recuperación es básica y no hay chequeo
de tipos. Está documentado en el README.

---

## 5. Demostración sugerida (5 minutos)

```bash
# 1. Compilar
cmake -S . -B build && cmake --build build -j

# 2. Caso válido mínimo
./build/analizador casos_de_prueba/valido_basico.lp

# 3. Caso válido con arreglos y expresiones
./build/analizador casos_de_prueba/valido_completo.lp --sin-tokens

# 4. Error clásico: falta ';'
./build/analizador casos_de_prueba/invalido_falta_punto_coma.lp \
    --sin-tokens --sin-traza --sin-tabla

# 5. Pruebas automatizadas
./pruebas/ejecutar_pruebas.sh
```

Durante la demo conviene señalar:

- La tabla de tokens demuestra la integración con el lexer.
- La traza muestra cada producción del entregable.
- El diagnóstico incluye línea, columna, fragmento y sugerencia.
- El código de salida permite usar el analizador en scripts de prueba.
