int x[2];

int sum(int x, int y) {
    int a;
    int b;
    a = x - 1;
    b = y + 1;
    return a + b;
}

int main(void) {
    int z;
    z = sum(3, 4);
    x[1] = z;

    if (z > 5) {
        $(0xFFFFFC60) = z;
    } else {
        $(0xFFFFFC60) = 0;
    }

    return 0;
}