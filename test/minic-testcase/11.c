
int main(void) {
    int a;
    int b;

    a = 3;
    b = 4;

    {
        int a;
        int b;

        a = 5;
        b = 6;
    }

    return 0;
}