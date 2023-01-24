int arr_glb[10];

void print(int arr[10], int x) {
    int i;
    i = 0;
    while (i < 10) {
        arr[i] = arr[i] + 3;
    }
    return;
}

int main(void) {
    int arr[10];
    int i;
    
    i = 0;
    while (i < 10) {
        arr[i] = i * 2;
        i = i + 1;
    }

    print(arr, arr[0]);

    return 0;
}