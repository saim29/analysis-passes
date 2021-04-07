/*
    ECE5984 Assignment 3
    DCE micro benchmark 2

*/
int main(){
    int a, b, c;

    a = 1;
    b = 2;
    c = 3;

    if(a)
    {
        a = 5;
    }
    else
    {
        a = 6;
    }

    a = b + 1;
    b = b + 2;
    c = a + c;

    return c;
}
