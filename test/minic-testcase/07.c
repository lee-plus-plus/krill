// calculate the sum of fiboonaci[1 ... 20]
int fib[20];
int glb;

int add(int x, int y) {
	int sum;
	sum = x + y;
	return sum;
}

int main(void) {
	int len;
	int i;

	len = 20;
	fib[0] = 1;
	fib[1] = 1;
	i = 2;
	while (i < len) {
		fib[i] = fib[i - 1] + fib[i - 2];
		i = i + 1;
	}

	glb = 0;
	i = 0;
	while (i < len) {
		glb = add(glb, fib[i]);
		i = i + 1;
	}
	return 0;
}


