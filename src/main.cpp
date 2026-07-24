// first, lets make terminal with custom instructions, and quit with q, then restore original terminal and exit
#include <termios.h>
#include <unistd.h>
#include <cstdlib>
#include <iostream>

namespace setup
{
    struct termios originalTerminal;
    int enableRawMode()
    {
        if (tcgetattr(STDIN_FILENO, &originalTerminal)  == -1) return -1;
        struct termios raw = originalTerminal;
        
        raw.c_iflag &= ~static_cast<tcflag_t>((IXON | ICRNL | BRKINT | ISTRIP | INPCK));
        raw.c_oflag &= ~static_cast<tcflag_t>((OPOST));
        raw.c_lflag &= ~static_cast<tcflag_t>((ECHO | ICANON | ISIG | IEXTEN));
        raw.c_cflag |= (CS8);
        raw.c_cc[VMIN] = 0; //minimum characters before read return
        raw.c_cc[VTIME] = 0; //time after which read returns if no input

        if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return -1;
        return 0;
    }

    int disableRawMode()
    {
        return tcsetattr(STDIN_FILENO, TCSAFLUSH, &originalTerminal);
    }

    void closeGame()
    {
        int code = disableRawMode();
        std::cout << "Thank you for playing" << std::endl;
        exit(code); // assumption here is every code apart from zero shows an error
    }

    int init()
    {
        return enableRawMode();
    }
}

namespace game
{
    void handleInput(char c)
    {
        if (c == 'q') setup::closeGame();
    }
}

int main()
{
    if (setup::init() == -1) setup::closeGame();

    char c = 0;
    for (;;)
    {
        if (read(STDOUT_FILENO, &c, 1) != 1) {}
        else {
            game::handleInput(c);
        }
    }
    return 0;
}
