int resutl;


int func2(int x, int y, int z) {
    int a;
    a = x - y;
    return a;
}

int func(int x, int y) {
    int z;
    z = x + y;
    z = z + func2(x, y, x);
    return z;
}

int main(void) {
    int z;
    z = func(3, 4);
    resutl = z;
    return 0;
}