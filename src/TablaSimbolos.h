#ifndef TABLA_SIMBOLOS_H
#define TABLA_SIMBOLOS_H

#include <string>
#include <vector>
#include <algorithm>

class TablaSimbolos {
private:
    std::vector<std::string> simbolos;

public:
    void insertar(const std::string& lexema) {
        if (std::find(simbolos.begin(), simbolos.end(), lexema) == simbolos.end()) {
            simbolos.push_back(lexema);
        }
    }

    // Permite acceder a la lista completa de identificadores para la impresión en paralelo
    const std::vector<std::string>& obtenerSimbolos() const {
        return simbolos;
    }
};

#endif