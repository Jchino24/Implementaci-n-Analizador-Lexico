#ifndef TABLA_SIMBOLOS_H
#define TABLA_SIMBOLOS_H

#include <cstddef>
#include <string>
#include <vector>

// Entrada de la tabla de símbolos: cada variable declarada con su tipo.
struct Simbolo {
    std::string nombre;
    std::string tipo;  // int, float, float[10], int[3][3], ...
    int linea = 0;
    int columna = 0;
};

class TablaSimbolos {
private:
    std::vector<Simbolo> simbolos;

public:
    // Devuelve false si el identificador ya estaba declarado.
    bool insertar(const Simbolo& simbolo) {
        if (buscar(simbolo.nombre) != nullptr) {
            return false;
        }
        simbolos.push_back(simbolo);
        return true;
    }

    const Simbolo* buscar(const std::string& nombre) const {
        for (const Simbolo& simbolo : simbolos) {
            if (simbolo.nombre == nombre) {
                return &simbolo;
            }
        }
        return nullptr;
    }

    const std::vector<Simbolo>& obtenerSimbolos() const {
        return simbolos;
    }

    std::size_t tamano() const {
        return simbolos.size();
    }
};

#endif  // TABLA_SIMBOLOS_H
