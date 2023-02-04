// calculate the sum of fiboonaci[1 ... 20]
int glb;
int stack_size = 0;
int stack_[100];

int fib(int i) {
    stack_[stack_size] = i;
    stack_size = stack_size + 1;
    if (i == 1) {
        return 1;
    } else if (i == 2) {
        return 1;
    } else {
        return fib(i - 1) + fib(i - 2);
    }
}

int main(void) {
    glb = fib(10);

    while (1);
    return 0;
}


