#include <stdio.h>
void test(char *p) {
    *p = '1';  //由于字符串不可变，所以报错(interrupted by signal 11: SIGSEGV)
    printf("%s",p);
}



int main(){
    char * p = "123";
    //test(p);

    printf("%c\n","123456"[4]);
    printf("%c\n",*("123456"+2));
    printf("%c\n","123456");

    return 0;
}



