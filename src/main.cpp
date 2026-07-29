// first, lets make terminal with custom instructions, and quit with q, then restore original terminal and exit
// then lets make the terminal show something! the game interface, at least a slab and the ball
// need to implement camera view port, that will be rendering just part of the level, and following the player
// will need to animate the environment, not to be so static.
// game state, will see in coming days
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <utility>
#include <string>
#include <sys/ioctl.h>
#include <vector>
#include <spanstream>
#include <string_view>

/*
 * screen class,
 * initialize it, enable raw mode, save terminal stuff, hide cursor,
 * clear screen, move cursor to topleft, move cursor to any spot row col, 
 * draw on screen
 * destructor to restore previous state
 *
 */
class Screen
{
    public:
        Screen();
        ~Screen();
        void clearScreen();
        bool enableRawMode();
        bool disableRawMode();
        void enableAltBuffer();
        void disableAltBuffer();
        void hideCursor();
        void restoreCursor();
        void cursorToTopLeft();
        void moveCursor(int row, int col);
        void draw(std::string&); // to be figured out;
        //handle when screen size changes during game
        std::pair<int, int> getWindowSize();
        std::pair<int, int> retrieveWindowSize();

        //temp
        friend std::ostream& operator<<(std::ostream&, Screen&);
    private:
        int winrows;
        int wincols;
        struct termios originalTerminal;
};

class Game
{
    private:
        std::vector<std::string_view> levels{};
        int currentLevel{};
        int cellsperpoint{};
        std::pair<int, int> writingPoint{1,1};
        Screen& screenref;

    public:
        Game(Screen&);
        ~Game();
        void draw();
};

struct point
{
    point(int x, int y) : row(x), col(y) {};
    int row;
    int col;
};

class Character
{
    private:
        struct point startingpoint;
        struct point currentpoint;

    public:

};

int main()
{
    Screen S;
    Game G(S);
    G.draw();
    char c = 0;
    for (;;)
    {
        if (read(STDOUT_FILENO, &c, 1) != 1) {}
        else {
            if (c == 'q') break;
        }
    }
    return 0;
}

Game::Game(Screen& s): screenref(s)
{
    cellsperpoint = 4; 
    currentLevel = 2;
    levels.push_back(std::string_view{"\
###########\n\
##   ######\n\
##   ######\n\
##   ######\n\
##   ######\n\
##         \n\
##@      | \n\
###########"});
    levels.push_back(std::string_view{"\
......................\n\
..#................#..\n\
..#..............=.#..\n\
..#.........o.o....#..\n\
..#.@......#####...#..\n\
..#####............#..\n\
......#++++++++++++#..\n\
......##############..\n\
......................"});
}

Game::~Game(){};

void Game::draw()
{
    std::string buffer{};
    std::ispanstream level{this->levels[(size_t)this->currentLevel - 1]};
    
    // need to improve this, thinking is, we avoid getline, instead loop through the level, saving lines, watching for \n chars, on \n, duplicate line 
    for (std::string line; std::getline(level, line);)
    {
        for (int i = 0; i < cellsperpoint; ++i)
        {
            for (size_t n = 0; n < line.length(); ++n)
            {
                for (int j = 0; j < cellsperpoint + 2; ++j)
                {
                    switch(line[n])
                        {
                            case '#':
                                buffer += "\033[48;5;234m \033[0m";
                                break;
                            case ' ':
                            case '.':
                                buffer += "\033[48;5;244m \033[0m";
                                break;
                            case '+':
                                buffer += "\033[48;5;88m \033[0m";
                                break;
                            case 'o':
                                buffer += "\033[48;5;208m \033[0m";
                                break;
                            default:
                                buffer += ' ';
                        }
                }
            }
            buffer += "\r\n";
        }
    }
    this->screenref.draw(buffer);
}

void Screen::draw(std::string& str)
{
    const float fillpercent = 0.6f; //60%
    // need to put constraints for when window size is to small, to ignore the percentage and fill most of/ all of the screen

    // at the very least, we seem to require 18 rows and 8 cols, of course we should be able to play with less, ( given the view port to be implemented )
    int cc = static_cast<int>(((1.0f - fillpercent) / 2.0f) * (float)wincols); 
    int r = static_cast<int>(((1.0f - fillpercent) / 2.0f) * (float)winrows); 

    moveCursor(r, cc);
    // here improve by making text then sending to output at once, not char by char
    std::for_each(str.cbegin(), str.cend(), [&](const char c) {
                if (c == '\n')
                {
                    moveCursor(++r, cc);
                }
                write(STDOUT_FILENO, &c, 1);
            });
}

Screen::Screen(): winrows{}, wincols{}, originalTerminal{}
{
    enableRawMode();
    enableAltBuffer();
    hideCursor();
    cursorToTopLeft();
    std::pair<int, int> winsize = retrieveWindowSize();
    winrows = winsize.first;
    wincols = winsize.second;

    //tmp
    //paint whole screen white-ish
    write(STDOUT_FILENO, "\033[48;5;244m", 11);
    clearScreen();
}

Screen::~Screen()
{
    disableRawMode();
    disableAltBuffer();
    restoreCursor();
}

std::ostream& operator<<(std::ostream& os, Screen& s)
{
    os << "window size:" << "\r\n";
    os << "rows: " << s.winrows << "\r\n";
    os << "cols: " << s.wincols << "\r\n";
    return os;
}

void Screen::clearScreen()
{
    write(STDOUT_FILENO, "\033[2J", 4);
    // check if 4 chars are written to error handle
}

void Screen::hideCursor()
{
    write(STDOUT_FILENO, "\033[?25l", 6); 
    // handle error
}

void Screen::restoreCursor()
{
    write(STDOUT_FILENO, "\033[?25h", 6);
    // handle error
}

/*
 * x and y are 1 based
 */
void Screen::moveCursor(int row, int col)
{
    std::string str{"\033["};
    str += std::to_string(row) + ';' + std::to_string(col) + 'H';
    write(STDOUT_FILENO, str.c_str(), str.length());
}

void Screen::cursorToTopLeft() { moveCursor(1,1); }

bool Screen::enableRawMode()
{
    if (tcgetattr(STDIN_FILENO, &this->originalTerminal)  == -1) return false;
    struct termios raw = originalTerminal;
       
    raw.c_iflag &= ~static_cast<tcflag_t>((IXON | ICRNL | BRKINT | ISTRIP | INPCK));
    raw.c_oflag &= ~static_cast<tcflag_t>((OPOST));
    raw.c_lflag &= ~static_cast<tcflag_t>((ECHO | ICANON | ISIG | IEXTEN));
    raw.c_cflag |= (CS8);
    raw.c_cc[VMIN] = 0; //minimum characters before read return
    raw.c_cc[VTIME] = 1; //time after which read returns if no input

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return false;
    return true;
}

bool Screen::disableRawMode()
{
    return tcsetattr(STDIN_FILENO, TCSAFLUSH, &this->originalTerminal) != -1;
}

void Screen::enableAltBuffer()
{
    write(STDOUT_FILENO, "\033[?1049h", 8);
    // handle error
}

void Screen::disableAltBuffer()
{
    write(STDOUT_FILENO, "\033[?1049l", 8);
    // handle error
}

std::pair<int, int> Screen::retrieveWindowSize()
{
    struct winsize ws;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
    return std::pair<int, int>{ws.ws_row, ws.ws_col};
}

std::pair<int, int> Screen::getWindowSize()
{
    return std::pair<int, int>{winrows, wincols};
}
