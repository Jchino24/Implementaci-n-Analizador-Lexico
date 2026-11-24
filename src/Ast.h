#ifndef AST_H
#define AST_H

#include <ostream>
#include <string>
#include <utility>
#include <vector>

// Nodo generico del AST. `tipo` indica la construccion y `valor` el nombre,
// operador o literal asociado.
struct AstNodo {
    std::string tipo;
    std::string valor;
    std::vector<AstNodo> hijos;

    AstNodo() = default;
    explicit AstNodo(std::string t, std::string v = "")
        : tipo(std::move(t)), valor(std::move(v)) {}

    void agregar(AstNodo hijo) {
        hijos.push_back(std::move(hijo));
    }
};

inline void imprimirAst(const AstNodo& nodo, std::ostream& salida, int nivel = 0) {
    salida << std::string(static_cast<std::size_t>(nivel) * 2, ' ') << nodo.tipo;
    if (!nodo.valor.empty()) {
        salida << ": " << nodo.valor;
    }
    salida << "\n";
    for (const AstNodo& hijo : nodo.hijos) {
        imprimirAst(hijo, salida, nivel + 1);
    }
}

#endif  // AST_H
