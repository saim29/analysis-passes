#include <stdio.h>

/*

    ECE5984 Assignment 3
    LICM micro benchmark 2
    This tests nested loops

*/
int main()
{

    int a,b,c,d,e;

    a = 20;
    b = 1000000;
    c = 5;
    d = 100;

    // nested while
    while (a<b) {

        a += a;
        for (int i=0; i<d; i++) {

            c = 100 + d;
            e = a + 10;

        }

    }


    return 0;
}