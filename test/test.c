// STDC functions
void* malloc(long);
void free(void*);
int printf(char*, ...);
int scanf(char*, ...);
void exit(int);
int putchar(int);
int rand();
void srand(int);

// The index of current assertion.
// Starts at 1.
int cnt;

// Asserts x == y.
void assert(int x, int y) {
    cnt++;
    if (x == y)
        return;
    
    printf("Assertion #%d failed: received %d, but should be %d\n", cnt, x, y);
    exit(1);
}

// Factorial of x.
int fact(int x) {
    if (x == 1)
        return 1;
    return x * fact(x - 1);
}

void swap(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}

int main() {
    // Calculation
    assert(1, 1);
    assert(1 + 2 * 3, 7);
    assert(5 * (5 - 4) + 3, 8);
    assert(-1 * -2, 2);
    assert(- - +-+ + +-3, 3);
    assert(((((((((((((1)))))))))))), 1);
    assert(16 % (3 - 2), 0);
    assert(7 / 5 + 11 * 8 - 32 % 17, 74);

    // Comparison
    assert(-1 < 0, 1);
    assert(1 > 0, 1);
    assert(-1 == 0, 0);
    assert(1 > -1, 1);
    assert(1 != 1, 0);

    // Variable operation
    int a = 0;
    assert(a, a);

    a += 2;
    assert(a, 2);

    a = a * 108;
    assert(a, 216);

    --a;
    assert(a, 215);

    // Loop
    int b = 0;
    for (int i = 0; i < 10; i++)
        b += i;
    assert(b, 45);

    while (a--)
        b += a;
    assert(b, 23050);

    // Recursion
    assert(fact(10), 3628800);

    // Pointer
    int* p = malloc(100);
    int* start = p;
    for (int i = 0; i < 25; i++)
        *p++ = i * i;

    int sum = 0;
    while (p != start - 1)
        sum += *p--;

    assert(sum, 4900);
    free(start);

    char* str = "Hello World!\n";
    while (*str)
        putchar(*str++);
    
    // Array
    int arr[20];
    srand(0);
    for (int i = 0; i < 20; i++)
        arr[i] = rand();
    // Bubble sort
    for (int i = 0; i < 20; i++)
        for (int j = i + 1; j < 20; j++)
            if (arr[i] > arr[j])
                swap(arr + i, arr + j);
    for (int i = 0; i < 19; i++)
        assert(arr[i] <= arr[i + 1], 1);

    printf("Everything is good!\n");
    return 0;
}