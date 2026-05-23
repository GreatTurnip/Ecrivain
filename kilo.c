/* INCLUDES */

#include<ctype.h>
#include<stdio.h>
#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<errno.h>

/* DEFINES */

#define CTRL_KEY(k) ((k) & 0x1f)

/* DATA */

struct termios orig_termios;

/* TERMINAL */

void die(char *s)
{
    perror(s);
    exit(1);
}
void disableRawMode()
{
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&orig_termios)==-1)
        die("tcsetattr");
}
void enableRawMode()
{
    atexit(disableRawMode);
    if(tcgetattr(STDIN_FILENO,&orig_termios) == -1)
        die("tcgetattr");
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(IXON | ICRNL);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= ~(CS8);
    raw.c_lflag &= ~(BRKINT | ECHO | ICANON | ISIG | IEXTEN);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) == -1)
        die("tcsetattr");
}

/* INIT */

int main()
{
    enableRawMode();
    while(1)
    {
        char c = '\0';
        if(read(STDIN_FILENO,&c,1) == -1)
            die("read");
        if(iscntrl(c))
        {
            printf("%d\r\n",c);
        }
        else
        {
            printf("%d'(%c)'\r\n",c,c);
        }
        if(c == CTRL_KEY('q'))
            break;
    }
    return 0;
}