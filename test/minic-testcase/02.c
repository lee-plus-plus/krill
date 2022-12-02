int main(void) {
	int a, b, c, d;
	a = 3 + 5 -7 *6 / 3;
	b = 5 << 3 * (6 >> 1);
	c = a + b;
	d = a & b | (a && b || c) ^ !a;
	e = (a > b) == (a >= 3) != (a < d) && (b <= e);
}