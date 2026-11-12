int suma(int a, int b) {
    int resultado;
    resultado = a + b;
    return resultado;
}

float promedio(float a, float b) {
    return (a + b) / 2;
}

void main() {
    int x;
    float p;
    x = func suma(4, 6);
    p = func promedio(15.5, 12.25);
    println(x);
    println(p);
}
