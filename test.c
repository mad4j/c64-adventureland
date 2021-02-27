#include <conio.h>


int main() {
    clrscr();

    bgcolor(COLOR_BLACK);
    bordercolor(COLOR_BLACK);
    textcolor(COLOR_YELLOW);

    cputs("Sweet80s\n\r");

    /* wait until a key is pressed or ellapsed timeout seconds */
    while (!kbhit()) {

    }

    return 0;
}