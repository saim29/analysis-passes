

#include <stdio.h>
int test2(int n) {
  int i;
  int c = 5;
  for (i = 0; i < n; i += 1)
    {
     c = n + 2;
    }
    return c;
}


int main()
{
    printf("%d",test2(-1));
    return 0;
}