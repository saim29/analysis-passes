#include <stdio.h>

/*
    ECE5984 Assignment 3
    LICM micro benchmark 3
    This is an extensive test with conditionals and nested loops
*/

int main () {

	int a, x, y, z;
	int res = 0;
    int q = 0;

    for (int i = 0; i < 100; i++) {

		a = 5;
		int k = i + a;

		if (i < 50) {

			y = a + 7;			
		}
		else {

			z = a + 8;

		}

        q = a * 5;
        
		for (int j = 0; j < 100; j++) {

			a = 10*q;
			x = 5+q;
			res += y;
            res += z;
		}

        res += res;
	}

	return 0;

}