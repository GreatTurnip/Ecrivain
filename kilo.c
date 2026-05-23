/* INCLUDES */

#include<ctype.h>
#include<stdio.h>
#include<unistd.h>
#include<termios.h>
#include<stdlib.h>
#include<errno.h>
#include<sys/ioctl.h>
#include<string.h>

/* DEFINES */

#define CTRL_KEY(k) ((k) & 0x1f)
#define ABUF_INIT {NULL, 0}

/* DATA */

struct editorconfig{
    struct termios orig_termios;
    int screenrows;
    int screencols;
    int cx,cy;
};

struct editorconfig E;

/* FORWARDS */

void die(char *s);
void disableRawMode();
void editorRefreshScreen();
void editorDrawRows();
void editorProcessKeypress();
int getWindowSize();
void initEditor();

/* APPEND BUFFER */

struct abuf {
    char *b;
    int len;
};

void abAppend(struct abuf *ab, char *s, int len)
{
    char* new = realloc(ab->b, ab->len + len);
    if(new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->b = new;
    ab->len += len;
}

void abFree(struct abuf *ab)
{
    free(ab->b);
}

/* TERMINAL */

void die(char *s)
{
    disableRawMode();
    perror(s);
    exit(1);
}
void disableRawMode()
{
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&E.orig_termios)==-1)
        die("tcsetattr");
}
void enableRawMode()
{
    atexit(disableRawMode);
    if(tcgetattr(STDIN_FILENO,&E.orig_termios) == -1)
        die("tcgetattr");
    struct termios raw = E.orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= CS8;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;
    if(tcsetattr(STDIN_FILENO,TCSAFLUSH,&raw) == -1)
        die("tcsetattr");
}

char editorReadKey()
{
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if(nread == -1) die("read");
    }
    return c;
}

void editorProcessKeypress()
{
    char c = editorReadKey();

    switch(c)
    {
        case CTRL_KEY('q'):
            exit(0);
            break;
    }
}

void editorRefreshScreen()
{
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    abAppend(&ab, "\x1b[H", 3);
    abAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.b, ab.len);
    abFree(&ab);
}

void editorDrawRows(struct abuf *ab)
{
    int y;
    for(y = 0; y < E.screenrows; y++)
    {
        abAppend(ab, "~", 1);
        abAppend(ab, "\x1b[K", 3);

        if(y < E.screenrows - 1)
        {
            abAppend(ab, "\r\n", 2);
        }
    }
}

void initEditor()
{
    E.cx = 0;
    E.cy = 0;
    if(getWindowSize(&E.screenrows, &E.screencols) == -1)   die("getWindowSize");
}

int getWindowSize(int* rows, int* col)
{
    struct winsize ws;
    if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    {
        return -1;
    }
    else
    {
        *rows = ws.ws_row;
        *col = ws.ws_col;
        return 0;
    }
}

/* INIT */

int main()
{
    enableRawMode();
    initEditor();
    while(1)
    {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}