#include <stdio.h>
#include <string.h>

struct test {
        char str[5];
};
        
int main(int argc, char const *argv[])
{
        struct test d;
        memset(&d, 0, sizeof(d));
        printf("%d\n", (int)strlen(d.str));
        return 0;
}
