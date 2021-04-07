/*
    ECE5984 Assignment 3
    DCE micro benchmark 1

*/
int main(void) {
    int a, b, c, d, e;

    a = 100;
    b = 5;
    c = 25;

    a +=a;
    d = a + c * 4;
    b+=b;
    e = a/c;

    return e;
}
