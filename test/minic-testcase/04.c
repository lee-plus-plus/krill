
void test1(int a) {
	if (a) a=1; else a=2;
	if (a) {return 1;}
	if (a) return 1; else return 2;
}

void test2(int b) {
	while (b) {
		b--;
		continue;
		break;
	}
	while (b) {
		b--;
	}
}

void test3(int c) {
	int x, y, z;
}

int main(void) {
	int a, b, c;
	test1(a);
	test2(b);
	test3(c);
}