// first, lets make terminal with custom instructions, and quit with q, then restore original terminal and exit
// then lets make the terminal show something! the game interface, at least a slab and the ball
// need to implement camera view port, that will be rendering just part of the level, and following the player
// will need to animate the environment, not to be so static.
// game state, will see in coming days
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <utility>
#include <format>
#include <string>
#include <sys/ioctl.h>
#include <vector>
#include <sstream>

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
        std::vector<std::string> levels{};
        int currentLevel{};

    public:
        Game();
        ~Game();
        void draw(Screen&);
};


int main()
{
    Screen S;
    Game G;
    G.draw(S);
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

Game::Game() 
{
    currentLevel = 1;
    levels.push_back(std::string{"\
###########\n\
##   ######\n\
##   ######\n\
##   ######\n\
##   ######\n\
##         \n\
##@      | \n\
###########"});
}

Game::~Game(){};

void Game::draw(Screen& s)
{
    std::string buffer{};
    const int cellsperpoint = 8;
    std::istringstream level{this->levels[(long unsigned int)this->currentLevel - 1]};
    
    for (std::string line; std::getline(level, line);)
    {
        for (int i = 0; i < cellsperpoint; ++i)
        {
            for (size_t n = 0; n < line.length(); ++n)
            {
                for (int j = 0; j < cellsperpoint; ++j)
                {
                    switch(line[n])
                        {
                            case '#':
                                buffer += "\033[48;5;196m \033[0m";
                                break;
                            case ' ':
                                buffer += "\033[48;5;251m \033[0m";
                                break;
                            default:
                                buffer += ' ';
                        }
                }
            }
            buffer += "\r\n";
        }
    }
    s.draw(buffer);
}

void Screen::draw(std::string& str)
{
    std::cout << str;
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
    std::string str = std::format("\033[{};{}H", row, col);
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

