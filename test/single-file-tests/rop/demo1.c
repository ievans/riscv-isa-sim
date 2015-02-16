int main();

//#include "tag-extensions.h"

int g() {
    return 4;
}

int f() {
    int x[10];
    int i;
    g();
    x[12] = (0xdeadbeef);
    return 0;
}

int main() {
    //tag_enforcement_on();
    f();
    return 0;
}
