int a;
int b[10];
int c;

int main(void) {
	int a;
	int b;
	int c;
	int d;
	int e;
	int f[10];
	a = 3 + 5 -7 *6 / 3;
	b = 5 << 3 * (6 >> 1);
	c = a + b;
	d = a & b | (a && b || c) ^ !a;
	e = (a > b) == (a >= 3) != (a < d) && (b <= e);
	// f[5] = !!a + --b + ++c + $f[3];
}