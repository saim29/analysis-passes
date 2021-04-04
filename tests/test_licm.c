#include <stdio.h>

int test(int x)
{
    int p = 0;
    int y = 100;
    int z = 20;

    z += p;
    while(1) {

        if (p>100)
            break;

        if (y>100)
            break;

        // if (p< 50)
        //     p =x*2;
        // else if (p>50)
        //     p = x*4;

        p += p;
        y = z + 20;
    }

    
    return 0;
}

int main()
{
    int r =test(3);
    printf("result is: %d\n",r);
    return 0;
}