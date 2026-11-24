# Informe técnico – Entregable 2

## 1. Arquitectura

```text
archivo .lp
   |
   v
Lexer  ->  vector<Token>  ->  Parser  ->  AST + errores
                                      |
                                      v
                              main.cpp (salida)
```

| Módulo | Responsabilidad |
|---|---|
| `Token.h/.cpp` | Tipos de token y nombres legibles. |
| `Lexer.h/.cpp` | Convierte caracteres en tokens, reconocimiento de comentarios, números, textos, operadores y palabras reservadas. |
| `Ast.h` | Define `AstNodo` y la impresión jerárquica del AST. |
| `Parser.h/.cpp` | Descenso recursivo, construcción del AST, errores y recuperación. |
| `main.cpp` | CLI, reporte de tokens, AST, errores y resultado. |

El parser **no lee caracteres**: consume el vector de tokens producido por el
lexer. Las líneas del archivo se leen solo para mostrar el fragmento y el caret
de los errores.

## 2. Decisiones de implementación

- **Descenso recursivo:** cada no terminal tiene una función con su nombre
  (`reconocerPrograma`, `reconocerSeleccion`, `reconocerMientras`, etc.).
- **Un token de lookahead:** `actual()` y `siguiente()` permiten distinguir
  funciones de `main` y elegir alternativas.
- **AST genérico:** `AstNodo` tiene `tipo`, `valor` e `hijos`; evita una clase
  por nodo y facilita imprimir el árbol.
- **AST construido durante el análisis:** las funciones de expresión devuelven
  un nodo; las de comando/bloque lo agregan a su padre.
- **Errores con recuperación:** `sincronizar()` avanza hasta `;` o `}` para
  reportar más de un error cuando es viable.
- **Sintaxis separada de semántica:** no se validan declaraciones, tipos ni
  ejecución.

## 3. AST

Nodos principales:

- `Programa`: funciones y `Principal`.
- `Funcion`: `TipoRetorno`, `Parametros` y `Bloque`.
- `Principal`: bloque de `main`.
- `Bloque`: declaraciones y comandos.
- `Declaracion` / `Variable`.
- `Asignacion`.
- `If` con `Condicion`, `Entonces` y `Sino`.
- `While` con `Condicion` y `Cuerpo`.
- `Llamada`, `Retorno`, `Lectura`, `Escritura`.
- `Binaria`, `Unaria`, `Literal`, `Texto`, `Identificador` y
  `AccesoArreglo`.

Ejemplo de salida:

```text
Programa
  Principal: void
    Bloque
      Declaracion: int
        Variable: edad
      Asignacion: edad
        Literal: 20
      If
        Condicion
          Binaria: >=
            Identificador: edad
            Literal: 18
        Entonces
          Bloque
            Escritura
              Texto: Edad valida
```

## 4. Manejo de errores

Cada error guarda línea, columna, mensaje y sugerencia. El reporte incluye el
fragmento de código y un caret. Entre los casos cubiertos:

- `;`, `}`, `)` o `]` faltantes.
- Token inesperado.
- Declaración después de un comando.
- `return` sin expresión.
- `scanf` con constante.
- `println` con una expresión no permitida.
- Asignación a un elemento de arreglo.
- Llamada sin `func` o sin paréntesis.

## 5. Pruebas

- 13 programas válidos.
- 22 programas inválidos.
- 35 casos en total.

```bash
./pruebas/ejecutar_pruebas.sh
```

El script compila, ejecuta, verifica el código de salida y guarda la evidencia
completa en `evidencias/`.

## 6. Limitaciones conocidas

- No se ejecuta el programa LP.
- No hay análisis semántico: no se comprueban tipos, declaraciones ni
  existencia de funciones.
- El AST es una representación sintáctica, no un AST semántico con tipos.
- Las dimensiones son constantes enteras.
- La recuperación de errores es básica (sincronización por `;` y `}`).
- `void main()` se acepta por compatibilidad con el ejemplo del curso.

## 7. Distribución del trabajo

| Parte | Responsable sugerido |
|---|---|
| Lexer y tokens | Integrante 1 |
| Parser y gramática | Integrante 2 |
| AST y errores | Integrante 3 |
| Pruebas, evidencias y documentación | Integrante 4 |

Completar los nombres del equipo en esta tabla antes de la entrega.
