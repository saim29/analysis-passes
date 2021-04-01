#include <stdio.h>

int main() {

    int a = 100;
    int b = 1000;
    int c;

    for(int i=0; i<a; i++) {

       c = a + b;
    }

    for(int i=0; i<b; i++) {

        for(int j=i; j<b; j++) {

          c = a + b;

        }

    }

    if (c) {

        printf("abc");
    }

    return 0;
}