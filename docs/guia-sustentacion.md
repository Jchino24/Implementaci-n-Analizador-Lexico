# Guía de sustentación – Entregable 2

## 1. Flujo

```text
main.cpp
  -> Parser(ruta)
     -> Lexer::getNextToken()  (tokens)
     -> analizar()
        -> reconocerPrograma()
           -> reconocerListaFunciones()
           -> reconocerPrincipal()
              -> reconocerBloque()
                 -> reconocerListaVariables()
                 -> reconocerComandos()
        -> AST
```

El parser consume tokens, no caracteres. Las líneas solo se leen para mostrar
el fragmento de los errores.

## 2. Qué explicar de cada parte

### Lexer

- Convierte caracteres en tokens.
- Reconoce identificadores, números, textos, operadores, comentarios `//` y
  palabras reservadas (`func`, `main`, `if`, `while`, `return`, etc.).

### Parser

- Una función por no terminal.
- `reconocerPrograma`: `LISTA_FUNCIONES` y `PRINCIPAL`.
- `reconocerDeclaracionFuncion`: tipo de retorno, nombre, parámetros y bloque.
- `reconocerBloque`: primero variables y después comandos.
- `reconocerComando`: lectura, escritura, asignación, llamada, `if`, `while`,
  `return`.
- Expresiones: `||`, `&&`, relacionales, aditivas, multiplicativas, factor.

### AST

- `AstNodo` contiene `tipo`, `valor` e `hijos`.
- Se construye mientras se analiza.
- Las expresiones producen `Binaria`, `Unaria`, `Literal`, `Texto`,
  `Identificador` o `AccesoArreglo`.
- Los comandos producen `Asignacion`, `If`, `While`, `Llamada`, `Retorno`,
  `Lectura` o `Escritura`.

### Errores

- `errorActual` indica lo esperado y lo encontrado.
- `errorPuntoComa` sugiere la línea donde falta `;`.
- `sincronizar` permite reportar más de un error.
- El proceso devuelve `0` o `1`.

## 3. Demostración

```bash
./build/analizador casos_de_prueba/valido_ejemplo_5_2.lp
./build/analizador casos_de_prueba/valido_funciones.lp
./build/analizador casos_de_prueba/invalido_falta_punto_coma.lp
./pruebas/ejecutar_pruebas.sh
```

Mostrar:

1. Tokens.
2. AST.
3. Un error con fragmento y caret.
4. Resumen `35/35`.

## 4. Preguntas probables

**¿Dónde se integra el lexer?**
En el constructor de `Parser`, que llama a `getNextToken()` y llena `flujo`.

**¿Cómo se construye el AST?**
Cada función de producción crea un `AstNodo`; las expresiones retornan su nodo
y los comandos/bloques lo agregan al padre.

**¿Cómo se respeta la precedencia?**
Con una función por nivel: `O -> Y -> Relacional -> Aditiva -> Multiplicativa
-> Factor`.

**¿Cómo se detecta un `;` faltante?**
Con `consumir(PUNTO_COMA)`; si falla, se reporta el token encontrado y una
sugerencia.

**¿Cómo se recupera de un error?**
`sincronizar()` avanza hasta `;` o `}` y el bloque continúa.

**¿Se valida semántica?**
No. Solo se construye sintaxis y AST; no se comprueban tipos ni declaraciones.

**¿Por qué `func` no lleva `;`?**
Porque `FUNCION -> func id ( ARGUMENTOS )`; el `;` pertenece a `ASIGNACION`
cuando la llamada es el complemento.

**¿Por qué `void main()` se acepta?**
La gramática usa `main()`, pero el ejemplo del curso usa `void main()`; se
acepta `void` opcional por compatibilidad.
