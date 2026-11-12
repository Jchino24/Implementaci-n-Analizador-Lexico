void main() {
    int notas[5];
    int matriz[2][2];
    int i;
    i = 0;
    notas[0] = 18;
    matriz[1][1] = 20;
    while (i < 5) {
        notas[i] = i * 2;
        i = i + 1;
    }
    println(notas[0]);
}
