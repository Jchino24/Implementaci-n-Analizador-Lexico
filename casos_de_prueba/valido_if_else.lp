void main() {
    int edad;
    int categoria;
    edad = 20;
    if (edad >= 18) {
        categoria = 1;
    } else {
        categoria = 0;
    }
    if (categoria == 1 && edad < 65) {
        println("adulto");
    } else if (edad >= 65) {
        println("adulto mayor");
    } else {
        println("menor");
    }
}
