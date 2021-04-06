#include<stdio.h>
int test(int a, int b) {
    int c = 0;
    int d = 0;
    int e = 0;
    for(int i = 0; i < 200; i++) {
        c = a * b; 
        d = a + i; 
        e = b + i; 
    }
    int f = c + d + e;
    return f;
}

int main() {
    printf("%d\n",test(2, 3));
    return 0;
}