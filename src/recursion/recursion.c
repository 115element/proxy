#include <stdio.h>

//得到斐波那契数列的第n项是几
int f(int n) {
    if (n == 1) {
        return 1;
    } else if (n == 2) {
        return 1;
    } else {
        return (f(n - 1) + f(n - 2));
    }
}

int sum(int n) {
    if (n == 1) {
        return 1;
    } else {
        return sum(n - 1) + n;
    }
}


void hanoi(int n, char A, char B, char C) {
    if (n == 1) {
        printf("%c -> %c\n", A, C);
    } else {
        hanoi(n - 1, A, C, B);
        printf("%c -> %c\n", A, C);
        hanoi(n - 1, B, A, C);
    }
}


int main() {
    int i = f(6);
    printf("%d\n", i);
    int j = sum(100);
    printf("%d\n", j);

    hanoi(3, 'A', 'B', 'C');
    return 0;
}