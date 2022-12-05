
void test1(int a);
int test1(int a, int b, int c);

void test1(int a) {
	if (a) a=1; else a=2;
	if (a) {return 1;}
	if (a) {return 1;} else {return 2;}
	if (a) if (a) a=1; else a=2;
	if (a) {if (a) a=1; else {a=2;}}
	return;
}

int test2(int a, int b, int c) {
	while (a) a = b - 1;
	while (b) {
		b = b - 1;
		continue;
		break;
	}
	while (c) {
		b = b - 1;
	}
	return a + b * c;
}


int main(void) {
	int a;
	int b;
	test1(a);
	test2(b);
}