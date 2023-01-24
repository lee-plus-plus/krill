int queens_glb[10];

void print_queens(int queens[8]) {
    int i;
    while (i < 8) {
        queens_glb[i] = queens[i];
        i = i + 1;
    }
    return;
}

int is_legal(int queens[8], int size) {
    int i;
    int x;
    int y;
    int rows[8];
    int cols[8];
    int cross1[15];
    int cross2[15];

    i = 0;
    while (i < 8) {
        rows[i] = 0;
        cols[i] = 0;
        i = i + 1;
    }
    i = 0;
    while (i < 15) {
        cross1[i] = 0;
        cross2[i] = 0;
        i = i + 1;
    }

    y = 0;
    while (y < size) {
        x = queens[y];

        if (cols[x]) {
            return 0;
        }
        if (rows[y]) {
            return 0;
        }
        if (cross1[x + y]) {
            return 0;
        }
        if (cross2[x - y + 7]) {
            return 0;
        }
        cols[x] = 1;
        rows[y] = 1;
        cross1[x + y] = 1;
        cross2[x - y + 7] = 1;

        y = y + 1;
    }
    return 1;
}

// if failed return 1, else return 0
int solve_queens(int queens[8], int size) {
    int y;

    if (size == 8) {
        print_queens(queens);
        return 1;
    }

    y = 0;
    while (y < 8) {
        queens[size] = y;
        if (is_legal(queens, size + 1)) {
            if (solve_queens(queens, size + 1)) {
                return 1;
            }
        }
        y = y + 1;
    }
    return 0;
}

int main(void) {
    int queens[8];
    int success;

    success = solve_queens(queens, 0);
    return 0;
}