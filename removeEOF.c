#include <stdio.h>
  
int count_char(){
    FILE* fp;
    int count = 0;
    char c;
    fp = fopen("s.txt", "r");
    for (c = getc(fp); c != EOF; c = getc(fp))
        count++;
    fclose(fp);
    return count;
}

int main() {
    FILE *fileptr1, *fileptr2;
    char ch;
    int a, b = 0;
    a = count_char();
    fileptr1 = fopen("r.txt", "r");
    fileptr2 = fopen("x.txt", "w");
    ch = getc(fileptr1);
    while (ch != EOF) {
        b++;
        putc(ch, fileptr2);
        ch = getc(fileptr1);
        if (b == a) {
            break;
        }
    }
    fclose(fileptr1);
    fclose(fileptr2);
    remove("r.txt");
    rename("x.txt", "r.txt");
    return 0;
}