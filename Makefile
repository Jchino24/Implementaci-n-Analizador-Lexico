CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
SRC := $(wildcard src/*.cpp)
HDR := $(wildcard src/*.h)

.PHONY: all run test clean

all: analizador

analizador: $(SRC) $(HDR)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

run: analizador
	./analizador casos_de_prueba/valido_basico.lp

test: analizador
	./pruebas/ejecutar_pruebas.sh

clean:
	rm -f analizador
