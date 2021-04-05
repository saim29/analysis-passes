#include <stdio.h>

/*
    ECE5984 Assignment 3
    LICM micro benchmark 1
    This only tests un-nested loops

*/

int main()
{

    int a,b,c,d,e;

    a = 20;
    b = 1000000;
    c = 5;
    d = 100;

    // Un-nested while
    while (a<b) {

        a += a;
        c = 100 + d;
        e = c + 200;

    }

    int f,g,h,i,j;

    f = 20;
    g = 1000000;
    h = 5;
    i = 100;

    // un-nested for
    for(int a=0; a<g; a++) {

        j = f + h;
        i = a + f;

    }

    return 0;
}