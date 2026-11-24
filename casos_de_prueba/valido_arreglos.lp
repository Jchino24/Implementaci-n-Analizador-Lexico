main() {
    int[5] notas;
    int[2][2] matriz;
    int i;
    int x;
    i = 0;
    x = notas[i];
    x = matriz[1][1];
    while (i < 5) {
        x = x + notas[i];
        i = i + 1;
    }
    println(x);
}
