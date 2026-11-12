void main() {
    int i;
    int total;
    i = 0;
    total = 0;
    while (i < 3) {
        int j;
        j = 0;
        while (j < 2) {
            total = total + i * j;
            j = j + 1;
        }
        i = i + 1;
    }
    if (total > 0) {
        int auxiliar;
        auxiliar = total;
        println(auxiliar);
    }
}
