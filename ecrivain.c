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

enum editorKey{
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    PAGE_UP,
    PAGE_DOWN,
};
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
void editorMoveCursor(int key);

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

int editorReadKey()
{
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if(nread == -1) die("read");
    }
    if(c == '\x1b')
    {
        char seq[3];
        //check if it's just simple escape sequence
        if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if(seq[0]=='[')
        {
            switch(seq[1])
            {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
            }
        }
        return '\x1b';
    }
    else
    {
        return c;
    }
}

void editorProcessKeypress()
{
    int c = editorReadKey();

    switch(c)
    {
        case CTRL_KEY('q'):
            exit(0);
            break;
        
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
    }
}

void editorRefreshScreen()
{
    struct abuf ab = ABUF_INIT;

    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    editorDrawRows(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
    abAppend(&ab, buf, strlen(buf));
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

void editorMoveCursor(int key)
{
    switch(key)
    {
        case ARROW_LEFT:
            if(E.cx != 0)   E.cx--;
            break;
        case ARROW_RIGHT:
            if(E.cx != E.screencols -1) E.cx++;
            break;
        case ARROW_DOWN:
            if(E.cy != E.screenrows -1) E.cx++;
            break;
        case ARROW_UP:
            if(E.cy != 0)   E.cy--;
            break;
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