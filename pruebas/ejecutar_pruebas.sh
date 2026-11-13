#!/usr/bin/env bash
#
# Compila el proyecto (si hace falta) y ejecuta todos los casos .lp.
# Los archivos cuyo nombre empieza por "valido_" deben terminar con código 0.
# Los archivos cuyo nombre empieza por "invalido_" deben terminar con código 1.
#
# Uso:
#   ./pruebas/ejecutar_pruebas.sh
#   ./pruebas/ejecutar_pruebas.sh --recompilar

set -uo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT" || exit 2
BUILD="$ROOT/build"
BIN="$BUILD/analizador"
EVIDENCIAS="$ROOT/evidencias"
RESUMEN="$EVIDENCIAS/resumen.txt"

if [[ "${1:-}" == "--recompilar" || ! -x "$BIN" ]]; then
    echo "== Compilando el proyecto =="
    cmake -S "$ROOT" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release || exit 2
    cmake --build "$BUILD" -j || exit 2
    echo
fi

mkdir -p "$EVIDENCIAS"
{
    echo "RESUMEN DE PRUEBAS - ANALIZADOR LP (ENTREGABLE 1)"
    echo "Generado por: pruebas/ejecutar_pruebas.sh"
    echo "Fecha: $(date '+%Y-%m-%d %H:%M:%S')"
    echo "======================================================================"
} > "$RESUMEN"

total=0
aprobadas=0
fallidas=0

shopt -s nullglob
archivos=("$ROOT"/casos_de_prueba/*.lp)
shopt -u nullglob

if [[ ${#archivos[@]} -eq 0 ]]; then
    echo "ERROR: no hay archivos .lp en casos_de_prueba/."
    exit 2
fi

echo "== Ejecutando casos de prueba =="
printf '%-8s %-38s %-10s %-10s\n' "ESTADO" "ARCHIVO" "ESPERADO" "OBTENIDO"
printf '%-8s %-38s %-10s %-10s\n' "------" "-------" "--------" "--------"

for archivo in "${archivos[@]}"; do
    nombre="$(basename "$archivo" .lp)"
    esperado=0
    if [[ "$nombre" == invalido_* ]]; then
        esperado=1
    fi

    salida="$EVIDENCIAS/$nombre.txt"
    "$BIN" "casos_de_prueba/$nombre.lp" > "$salida" 2>&1
    obtenido=$?

    # Ademas del codigo de salida, se comprueba el veredicto impreso.
    veredicto_ok=1
    if [[ $esperado -eq 0 ]]; then
        grep -q "RESULTADO FINAL: PROGRAMA VÁLIDO" "$salida" || veredicto_ok=0
    else
        grep -q "RESULTADO FINAL: PROGRAMA CON ERRORES" "$salida" || veredicto_ok=0
    fi

    total=$((total + 1))
    if [[ $obtenido -eq $esperado && $veredicto_ok -eq 1 ]]; then
        estado="OK"
        aprobadas=$((aprobadas + 1))
    else
        estado="FALLO"
        fallidas=$((fallidas + 1))
    fi

    printf '%-8s %-38s %-10s %-10s\n' "$estado" "$nombre.lp" "$esperado" "$obtenido"
    printf '%-8s %-38s esperado=%s obtenido=%s salida=evidencias/%s.txt\n' \
        "$estado" "$nombre.lp" "$esperado" "$obtenido" "$nombre" >> "$RESUMEN"

    if [[ $obtenido -ne $esperado ]]; then
        echo "  Últimas líneas de $salida:"
        tail -n 8 "$salida" | sed 's/^/    /'
    fi
done

{
    echo "======================================================================"
    echo "Total de casos:  $total"
    echo "Aprobados:       $aprobadas"
    echo "Fallidos:        $fallidas"
    echo "Evidencias:      evidencias/"
    echo "======================================================================"
} | tee -a "$RESUMEN"

if [[ $fallidas -gt 0 ]]; then
    exit 1
fi
exit 0
