int func(int x, int y) {
   int a;
   a = x + y;
   return a;
}

void main(void) {
   int x;
   int y;
   x = 5;
   y = 3;
   x = func(x, y);
   x = x + 3;
   return;
}