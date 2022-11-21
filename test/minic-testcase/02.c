int main() {
	int a = 3 + 5 -7 *6 / 3;
	int b = (5 << 3) * (6 >> 1);
	int c = ++a + b--;
	int d = a & b | (a && b || c) ^ !a;
	int e = (a > b) == (a >= 3) != (a < d) && (b <= e);
}