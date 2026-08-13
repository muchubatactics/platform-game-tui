#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <utility>
#include <string>
#include <sys/ioctl.h>
#include <vector>
#include <string_view>
#include <fstream>

struct point
{
    point(int x, int y) : row{x}, col{y} {};
    int row;
    int col;
};

enum class CharacterType
{
    Player,
    Lava,
    Coin,
};

class Character
{
    private:
        const struct point startingpoint;
        struct point currentpoint;
        const struct point endingpoint;

    public:
        Character(struct point starting, struct point ending);
        Character(struct point starting);
        virtual ~Character() = default;
        virtual void updatePoint() = 0;
        struct point getCurrentPoint() const;
        virtual const std::string& getStrRepresentation() = 0;
        virtual CharacterType getType() = 0;
        void setCurrentPoint(int, int);
};

Character::Character(struct point start, struct point end):
    startingpoint{start}, currentpoint{start}, endingpoint{end}
{
    // reserve str?
    // do everything here
}

Character::Character(struct point start):
    Character(start, {-1, -1})
{};

struct point Character::getCurrentPoint() const
{
    return currentpoint;
}

//deprecated ( already )
std::string createStr(std::string color, std::string str, int scale)
{
    std::string s{str};
    for (int i = 1; i < scale; ++i)
    {
        s += str;
    }
    s = color + s + std::string{"\033[48;5;244m"};
    str = s;
    for (int i = 1; i < scale; ++i)
    {
        s += (std::string{"\n"} + str);
    }
    return s;
}

void Character::setCurrentPoint(int row, int col)
{
    currentpoint.row = row;
    currentpoint.col = col;
}

class Player: public Character
{
    private:
        int verticalforce{};
        int horizontalforce{};
        float gravity{9.8f};
        static inline const std::string str{"\033[48;5;21m \033[48;5;244m"};
        static inline CharacterType ctype = CharacterType::Player;

    public:
        Player(struct point start);
        void consumeKeyPress(char);
        // overides
        void updatePoint() override;
        const std::string& getStrRepresentation() override;
        CharacterType getType() override { return ctype; }

        //static
        static const std::string& getStr();
};

Player::Player(struct point start):
    Character{start}
{
}

const std::string& Player::getStrRepresentation()
{
    return str;
}

const std::string& Player::getStr()
{
    return str;
}

void Player::updatePoint()
{
    // naive impl
    struct point cur = getCurrentPoint();
    if (verticalforce)
    {
        if (verticalforce > 0)
        {
            verticalforce--;
            setCurrentPoint(--cur.row, cur.col);
        } else {
            verticalforce++;
            setCurrentPoint(++cur.row, cur.col);
        }
    }
    if (horizontalforce)
    {
        if (horizontalforce > 0)
        {
            horizontalforce--;
            setCurrentPoint(cur.row, ++cur.col);
        } else {
            horizontalforce++;
            setCurrentPoint(cur.row, --cur.col);
        }
    }
}

void Player::consumeKeyPress(char c)
{
    switch(c)
    {
        case 'w': 
            verticalforce++;
            break;
        case 'a':
            horizontalforce--;
            break;
        case 's':
            verticalforce--;
            break;
        case 'd':
            horizontalforce++;
            break;
        default:
            ;;
    }
}

class Lava: public Character
{
    private:
        int horizontalspeed;
        int verticalspeed;
        bool resets;
        static inline const std::string str{"\033[48;5;88m \033[48;5;244m"};
        static inline const CharacterType ctype = CharacterType::Lava;

    public:
        Lava(struct point s, int v, int h, bool resets);
        //overides
        void updatePoint() override;
        const std::string& getStrRepresentation() override;
        CharacterType getType() override { return ctype; }

        //static
        static const std::string& getStr();
};

Lava::Lava(struct point start, int vertical, int horizontal, bool resets):
    Character{start}, horizontalspeed{horizontal}, verticalspeed{vertical}, resets{resets}
{
}

void Lava::updatePoint()
{
    struct point cur = getCurrentPoint();
    if (horizontalspeed)
    {
        setCurrentPoint(cur.row, cur.col -= horizontalspeed);
    }
    if (verticalspeed)
    {
        setCurrentPoint(cur.row += verticalspeed, cur.col);
    }
}

const std::string& Lava::getStrRepresentation()
{
    return str;
}

const std::string& Lava::getStr()
{
    return str;
}

class Coin: public Character // questionable
{
    private:
        bool consumed{false};
        static inline const std::string str{"\033[48;5;208m \033[48;5;244m"};
        static inline const CharacterType ctype = CharacterType::Coin;

    public:
        Coin(struct point start);
        void setConsumed(bool);
        //overrides ?
        void updatePoint() override;
        const std::string& getStrRepresentation() override;
        CharacterType getType() override { return ctype; }

        //static
        static const std::string& getStr();
};

Coin::Coin(struct point start):
    Character{start}
{
}

const std::string& Coin::getStrRepresentation()
{
    return str;
}

const std::string& Coin::getStr()
{
    return str;
}

void Coin::setConsumed(bool val)
{
    consumed = val;
}

void Coin::updatePoint()
{
    if(consumed)
    {
        setCurrentPoint(-1, -1);
    }
}

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
        void draw(const std::string&);
        //handle when screen size changes during game
        std::pair<int, int> getWindowSize();
        std::pair<int, int> retrieveWindowSize();

        const std::string convertToScreenStr(std::string_view);
        void writeError(std::string_view);
        struct point convertToScrCoordinates(int, int, bool);

        // for easier logging
        std::ofstream& getLog();

        //temp
        friend std::ostream& operator<<(std::ostream&, Screen&);

    private:
        int winrows;
        int wincols;
        struct termios originalTerminal;
        std::string previousframe;
        int scale;
        // todo:: move this to own class and make it available for everyone;
        // or i can redirect std::cerr to the file then use that, since its already accessible everywhere
        std::ofstream logstream;
};

class Game
{
    private:
        std::vector<std::string_view> levels{};
        int currentLevel{};
        std::vector<Character*> characters{};
        std::string gamestate{};
        int coinsleft{};
        std::string::size_type currentLevelRowLength{};
        std::string::size_type currentLevelHeight{};
        Player* maincharacter{nullptr};

    public:
        Game();
        ~Game();
        void updateGameState();
        void paintCharactersInGameState();
        void consumeKey(char);
        void removeCharactersFromGameState();
        const std::string& getGameState();
        const Player* getMainCharacter();
        int getCharacterIndexInGameState(const Character*);
        std::string::size_type getCurrentLevelRowLength();
        std::string::size_type getCurrentLevelHeight();

        //disable copying
        Game(const Game&) = delete;
        Game& operator=(const Game&) = delete;
};

class ViewPort
{
    public:
        ViewPort(Screen&, Game&);
        ~ViewPort();
        void draw();

    private:
        Screen& screenref;
        Game& gameref;
};


int main()
{
    Screen S;
    Game G;
    ViewPort vp{S, G};
    vp.draw();
    char c = 0;
    for (;;)
    {
        if (read(STDOUT_FILENO, &c, 1) != 1) {}
        else {
            if (c == 'q') break;
            G.consumeKey(c);
        }
        G.updateGameState();
        vp.draw();
    }
    return 0;
}

ViewPort::ViewPort(Screen& s, Game& g): screenref{s}, gameref{g}
{}

ViewPort::~ViewPort()
{}

void ViewPort::draw()
{
    const std::string& cur = gameref.getGameState();

    // let target a 20 rows by 26 cols, for now
    // 6 14 6 for cols 
    // 7 6 7 for rows
    //
    // BE CAREFUL WE ARE USING LONG UNSIGNED INTS HERE, SUBTRACTING FROM ZERO WILL OVERFLOW TO A HUGE INT
    const std::string::size_type vpc = 26;
    const std::string::size_type vpr = 20;
    std::string::size_type bc = 0;
    std::string::size_type br = 0;
    const std::string::size_type l = gameref.getCurrentLevelRowLength(); // if this is n, it means theres n + 1 characters in the str per row, the + 1 being the \n character
    const std::string::size_type h = gameref.getCurrentLevelHeight(); // if this is n, it means theres n lines;
    
    struct point pnt = gameref.getMainCharacter()->getCurrentPoint();
   
    // pnt row col is 1 based, a relic from the tui winsize being 1 based
    if (vpc >= l)
    {
        bc = 0;
    }
    else if (pnt.col - 1 >= (int)l - 6 - 14)
    {
        bc = l - vpc;
    }
    else if (pnt.col - 1 <= 20)
    {
        bc = 0;
    }
    else
    {
        // can either be on 6 or on 6 + 14, depending on previous position
        // but lets initialize it to 6 for now
        bc = (std::string::size_type)pnt.col - 1 - 6;
    }

    if (vpr >= h)
    {
        br = 0;
    }
    else if (pnt.row - 1 >= (int)h - 7 - 6)
    {
        br = h - vpr;
    }
    else if (pnt.row - 1 <= 13)
    {
        br = 0;
    }
    else
    {
        br = (std::string::size_type)pnt.row - 1 - 7;
    }

    std::string sv{cur.substr(br * (l + 1))}; // why wont string_view work here ?
    // when we put movement this will become more complicated;
    std::string buffer{sv.substr(bc, vpc)};
    //reserve?
    //
    //this i is supposed to point to the first \n char after the line we just initialized the buffer with
    std::string::size_type i{l};

    for (std::string::size_type rr = 1; i < sv.size() && rr < vpr; i += l + 1, ++rr)
    {
        if (sv[i] == '\n' && i + 1 < sv.size())
        {
            buffer += '\n';
            buffer += sv.substr(i + 1 + bc, vpc);
        }
        else
        {
            ;;
        }
    }
    
    screenref.draw(buffer);
}

Game::Game()
{
    currentLevel = 3;

// braces to easily collapse this
{
    levels.push_back(std::string_view{"\
......................\n\
..#................#..\n\
..#..............=.#..\n\
..#.........o.o....#..\n\
..#.@......#####...#..\n\
..#####............#..\n\
......#++++++++++++#..\n\
......##############..\n\
......................\n"});

    levels.push_back(std::string_view{"\
................................................................................\n\
................................................................................\n\
................................................................................\n\
................................................................................\n\
................................................................................\n\
................................................................................\n\
..................................................................###...........\n\
...................................................##......##....##+##..........\n\
....................................o.o......##..................#+++#..........\n\
.................................................................##+##..........\n\
...................................#####..........................#v#...........\n\
............................................................................##..\n\
..##......................................o.o................................#..\n\
..#.....................o....................................................#..\n\
..#......................................#####.............................o.#..\n\
..#..........####.......o....................................................#..\n\
..#..@.......#..#................................................#####.......#..\n\
..############..###############...####################.....#######...#########..\n\
..............................#...#..................#.....#....................\n\
..............................#+++#..................#+++++#....................\n\
..............................#+++#..................#+++++#....................\n\
..............................#####..................#######....................\n\
................................................................................\n\
................................................................................\n"});

    levels.push_back(std::string_view{"\
................................................................................\n\
................................................................................\n\
....###############################.............................................\n\
...##.............................##########################################....\n\
...#.......................................................................##...\n\
...#....o...................................................................#...\n\
...#................................................=.......................#...\n\
...#.o........################...................o..o...........|........o..#...\n\
...#.........................#..............................................#...\n\
...#....o....................##########.....###################....##########...\n\
...#..................................#+++++#.................#....#............\n\
...###############....oo......=o.o.o..#######.###############.#....#............\n\
.....#...............o..o.............#.......#......#........#....#............\n\
.....#....................#############..######.####.#.########....########.....\n\
.....#.............########..............#...........#.#..................#.....\n\
.....#..........####......####...#####################.#..................#.....\n\
.....#........###............###.......................########....########.....\n\
.....#.......##................#########################......#....#............\n\
.....#.......#................................................#....#............\n\
.....###......................................................#....#............\n\
.......#...............o...........................................#............\n\
.......#...............................................o...........#............\n\
.......#########......###.....############.........................##...........\n\
.............#..................#........#####....#######.o.........########....\n\
.............#++++++++++++++++++#............#....#.....#..................#....\n\
.............#++++++++++++++++++#..........###....###...####.o.............#....\n\
.............####################..........#........#......#.....|.........#....\n\
...........................................#++++++++#......####............#....\n\
...........................................#++++++++#.........#........@...#....\n\
...........................................#++++++++#.........##############....\n\
...........................................##########...........................\n\
................................................................................\n"});

    levels.push_back(std::string_view{"\
......................................#++#........................#######....................................#+#..\n\
......................................#++#.....................####.....####.................................#+#..\n\
......................................#++##########...........##...........##................................#+#..\n\
......................................##++++++++++##.........##.............##...............................#+#..\n\
.......................................##########++#.........#....................................o...o...o..#+#..\n\
................................................##+#.........#.....o...o....................................##+#..\n\
.................................................#+#.........#................................###############++#..\n\
.................................................#v#.........#.....#...#........................++++++++++++++##..\n\
.............................................................##..|...|...|..##............#####################...\n\
..............................................................##+++++++++++##............v........................\n\
...............................................................####+++++####......................................\n\
...............................................#.....#............#######........###.........###..................\n\
...............................................#.....#...........................#.#.........#.#..................\n\
...............................................#.....#.............................#.........#....................\n\
...............................................#.....#.............................##........#....................\n\
...............................................##....#.............................#.........#....................\n\
...............................................#.....#......o..o.....#...#.........#.........#....................\n\
...............#######........###...###........#.....#...............#...#.........#.........#....................\n\
..............##.....##.........#...#..........#.....#.....######....#...#...#########.......#....................\n\
.............##.......##........#.o.#..........#....##...............#...#...#...............#....................\n\
.....@.......#.........#........#...#..........#.....#...............#...#...#...............#....................\n\
....###......#.........#........#...#..........#.....#...............#...#####...######......#....................\n\
....#.#......#.........#.......##.o.##.........#.....#...............#.....o.....#.#.........#....................\n\
++++#.#++++++#.........#++++++##.....##++++++++##....#++++++++++.....#.....=.....#.#.........#....................\n\
++++#.#++++++#.........#+++++##.......##########.....#+++++++##+.....#############.##..o.o..##....................\n\
++++#.#++++++#.........#+++++#....o.................##++++++##.+....................##.....##.....................\n\
++++#.#++++++#.........#+++++#.....................##++++++##..+.....................#######......................\n\
++++#.#++++++#.........#+++++##.......##############++++++##...+..................................................\n\
++++#.#++++++#.........#++++++#########++++++++++++++++++##....+..................................................\n\
++++#.#++++++#.........#++++++++++++++++++++++++++++++++##.....++++++++++++.......................................\n"});

    levels.push_back(std::string_view{"\
..............................................................................................................\n\
..............................................................................................................\n\
..............................................................................................................\n\
..............................................................................................................\n\
..............................................................................................................\n\
........................................o.....................................................................\n\
..............................................................................................................\n\
........................................#.....................................................................\n\
........................................#.....................................................................\n\
........................................#.....................................................................\n\
........................................#.....................................................................\n\
.......................................###....................................................................\n\
.......................................#.#.................+++........+++..###................................\n\
.......................................#.#.................+#+........+#+.....................................\n\
.....................................###.###................#..........#......................................\n\
......................................#...#.................#...oooo...#.......###............................\n\
......................................#...#.................#..........#......#+++#...........................\n\
......................................#...#.................############.......###............................\n\
.....................................##...##......#...#......#................................................\n\
......................................#...#########...########..............#.#...............................\n\
......................................#...#...........#....................#+++#..............................\n\
......................................#...#...........#.....................###...............................\n\
.....................................##...##..........#.......................................................\n\
......................................#...#=.=.=.=....#............###........................................\n\
......................................#...#...........#...........#+++#.......................................\n\
......................................#...#....=.=.=.=#.....o......###.......###..............................\n\
.....................................##...##..........#.....................#+++#.............................\n\
..............................o...o...#...#...........#.....#................##v........###...................\n\
......................................#...#...........#..............#.................#+++#..................\n\
.............................###.###.###.###.....o.o..#++++++++++++++#...................v#...................\n\
.............................#.###.#.#.###.#..........#++++++++++++++#........................................\n\
.............................#.............#...#######################........................................\n\
.............................##...........##.........................................###......................\n\
..###.........................#.....#.....#.........................................#+++#................###..\n\
..#.#.........................#....###....#..........................................###.................#.#..\n\
..#...........................#....###....#######........................#####.............................#..\n\
..#...........................#...........#..............................#...#.............................#..\n\
..#...........................##..........#..............................#.#.#.............................#..\n\
..#.......................................#.......|####|....|####|.....###.###.............................#..\n\
..#................###.............o.o....#..............................#.........###.....................#..\n\
..#...............#####.......##..........#.............................###.......#+++#..........#.........#..\n\
..#...............o###o.......#....###....#.............................#.#........###..........###........#..\n\
..#................###........#############..#.oo.#....#.oo.#....#.oo..##.##....................###........#..\n\
..#......@..........#.........#...........#++#....#++++#....#++++#....##...##....................#.........#..\n\
..#############################...........#############################.....################################..\n\
..............................................................................................................\n\
..............................................................................................................\n"});

    levels.push_back(std::string_view{"\
..................................................................................................###.#.......\n\
......................................................................................................#.......\n\
..................................................................................................#####.......\n\
..................................................................................................#...........\n\
..................................................................................................#.###.......\n\
..........................o.......................................................................#.#.#.......\n\
.............................................................................................o.o.o###.#.......\n\
...................###................................................................................#.......\n\
.......+..o..+................................................#####.#####.#####.#####.#####.#####.#####.......\n\
.......#.....#................................................#...#.#...#.#...#.#...#.#...#.#...#.#...........\n\
.......#=.o..#............#...................................###.#.###.#.###.#.###.#.###.#.###.#.#####.......\n\
.......#.....#..................................................#.#...#.#...#.#...#.#...#.#...#.#.....#.......\n\
.......+..o..+............o..................................####.#####.#####.#####.#####.#####.#######.......\n\
..............................................................................................................\n\
..........o..............###..............................##..................................................\n\
..............................................................................................................\n\
..............................................................................................................\n\
......................................................##......................................................\n\
...................###.........###............................................................................\n\
..............................................................................................................\n\
..........................o.....................................................#......#......................\n\
..........................................................##.....##...........................................\n\
.............###.........###.........###.................................#..................#.................\n\
..............................................................................................................\n\
.................................................................||...........................................\n\
..###########.................................................................................................\n\
..#.........#.o.#########.o.#########.o.##................................................#...................\n\
..#.........#...#.......#...#.......#...#.................||..................#.....#.........................\n\
..#..@......#####...o...#####...o...#####.....................................................................\n\
..#######.....................................#####.......##.....##.....###...................................\n\
........#=..................=................=#...#.....................###...................................\n\
........#######################################...#+++++++++++++++++++++###+++++++++++++++++++++++++++++++++++\n\
..................................................############################################################\n\
..............................................................................................................\n"});
}
    
    std::string buffer{};
    //reserve buffer ?
    
    std::string_view thislevel = levels[(size_t)(currentLevel - 1)];
    int rown = 1;
    int coln = 1;
    int coinnumber = 0;
    std::for_each(thislevel.cbegin(), thislevel.cend(), [&] (const char c)
            {
                if (c == '\n' || c == '\0')
                {
                    ++rown; coln = 1;
                    buffer += "\n";
                }
                else {
                    switch(c)
                    {
                        case '#':
                        case '+':
                        case '.':
                            buffer += c;
                            break;
                        case 'o':
                            {
                                ++coinnumber;
                                Coin* c = new Coin({rown, coln});
                                characters.push_back(c);
                                buffer += '.';
                                break;
                            }
                        case '=':
                            {
                                Lava* c = new Lava({rown, coln}, 0, 1, false);
                                characters.push_back(c);
                                buffer += '.';
                                break;
                            }
                        case '|':
                            {
                                Lava* c = new Lava({rown, coln}, 1, 0, false);
                                characters.push_back(c);
                                buffer += '.';
                                break;
                            }
                        case 'v':
                            {
                                Lava* c = new Lava({rown, coln}, 1, 0, true);
                                characters.push_back(c);
                                buffer += '.';
                                break;
                            }
                        case '@':
                            {
                                Player* c = new Player({rown, coln});
                                maincharacter = c;
                                characters.push_back(c);
                                buffer += '.';
                                break;
                            }
                        default:
                            buffer += '.';
                    }
                    ++coln;
                }
            });

    coinsleft = coinnumber;
    gamestate = buffer;
    currentLevelRowLength = thislevel.find('\n');
    currentLevelHeight = thislevel.size() / (currentLevelRowLength + 1);
    //draw the characters; maybe;
    paintCharactersInGameState();
}

std::string::size_type Game::getCurrentLevelRowLength()
{
    return currentLevelRowLength;
}

std::string::size_type Game::getCurrentLevelHeight()
{
    return currentLevelHeight;
}

const Player* Game::getMainCharacter()
{
    return maincharacter;
}

const std::string& Game::getGameState()
{
    return gamestate;
}

void Game::removeCharactersFromGameState()
{
    for (std::string::size_type i = 0; i < gamestate.size(); ++i)
    {
        switch(gamestate[i])
        {
            case '@':
            case '=':
            case 'o':
                gamestate[i] = '.';
            default:
                ;;
        }
    }
}

int Game::getCharacterIndexInGameState(const Character* cptr)
{
    // we are taking advantage of all lines being of equal length;
    // reducing it to (n + 1) * (r - 1) + (c - 1)
    std::string::size_type n = currentLevelRowLength;
    struct point pos = cptr->getCurrentPoint();

    if (!pos.row || !pos.col)
    {
        //error handle
        return -1;
    }
    if (pos.row == -1 && pos.col == -1)
    {
        //okay
        return -1;
    }
    if (pos.row < -1 || pos.col < 0)
    {
        // error handle
        return -1;
    }
    return ((static_cast<int>(n) + 1) * (pos.row - 1)) + (pos.col - 1);
}

void Game::paintCharactersInGameState()
{
    // remove existing characters
    removeCharactersFromGameState();
    for (Character* ptr : characters)
    {
        int i = getCharacterIndexInGameState(ptr);
        if (i == -1) continue; // invalid index 
        std::string::size_type index = (std::string::size_type)(i);
        if (index >= gamestate.length()) {} // BIG ERROR, handle it
        else
        {
            switch(ptr->getType())
            {
                case CharacterType::Lava:
                    gamestate[index] = '=';
                    break;
                case CharacterType::Player:
                    gamestate[index] = '@';
                    break;
                case CharacterType::Coin:
                    gamestate[index] = 'o';
                    break;
                default:
                    // error handle it
                    ;;
            }
        }

    }
}

void Game::updateGameState()
{
    for (Character* ptr : characters)
    {
        ptr->updatePoint();
    }
    paintCharactersInGameState();
}

Game::~Game()
{
    for (Character* c : characters)
    {
        delete c;
    }
}

void Game::consumeKey(char c)
{
    if (maincharacter) maincharacter->consumeKeyPress(c);
}

const std::string Screen::convertToScreenStr(std::string_view sv)
{
    std::string buffer{};
    std::string line{};
    std::for_each(sv.cbegin(), sv.cend(), [&] (const char c)
            {
                if (c == '\n' || c == '\0')
                {
                    for (int i = 0; i < scale; ++i)
                    {
                        buffer += line;
                        buffer += "\r\n";
                    }
                    line = "";
                }
                else {
                    switch(c)
                    {
                        case '#':
                            line += "\033[48;5;234m";
                            break;
                        case 'o':
                            line += "\033[48;5;208m";
                            break;
                        case '=':
                        case '|':
                        case 'v':
                        case '+':
                            line += "\033[48;5;88m";
                            break;
                        case '@':
                            line += "\033[48;5;21m";
                            break;
                        case '.':
                            line += "\033[48;5;244m";
                            break;
                        default:
                            ;;
                    }

                    line.append((std::string::size_type)(scale + (int)(scale / 2)), ' ');
                    line += "\033[48;5;244m";
                }
            });
    return buffer;
}

void Screen::writeError(std::string_view str)
{
    if (logstream.is_open())
    {
        logstream << str;
    }
    else
    {
        moveCursor(winrows, 0);
        write(STDOUT_FILENO, str.data(), str.size());
    }
}

/*
 * caller must remember to check if file is open
 */
std::ofstream& Screen::getLog()
{
    return logstream;
}

// given the viewport, should the screen still draw??
// probably not, but we shall see
// for now i have decided that screen should be the one to handle drawing to the screen (duh)
// viewport is only concerned with making a substr of the gamestate str and asking the screen to draw it
void Screen::draw(const std::string& str)
{
//    const float fillpercent = 0.6f; //60%
    // need to put constraints for when window size is to small, to ignore the percentage and fill most of/ all of the screen

    // at the very least, we seem to require 18 rows and 8 cols, of course we should be able to play with less, ( given the view port to be implemented )
//    int cc = static_cast<int>(((1.0f - fillpercent) / 2.0f) * (float)wincols); 
//    int r = static_cast<int>(((1.0f - fillpercent) / 2.0f) * (float)winrows); 

//    moveCursor(r, cc);
    // here improve by making text then sending to output at once, not char by char
//    std::for_each(str.cbegin(), str.cend(), [&](const char c) {
//                if (c == '\n')
//                {
//                    moveCursor(++r, cc);
//                }
//                write(STDOUT_FILENO, &c, 1);
//            });
//
    // here we are having a challenge, if we want to print the string at once, we need to insert move cursor ascii codes after every line
    // for now, ill just print to the top left, till i implement the viewport
    
    //need to first clear screen
    //clearScreen(); // since we always write in the same place, theres no need to clear screen;


 // consider that stuff of std::move when implementing this

// new impl
    std::string tmp{};
    if (previousframe.empty())
    {
        tmp += "\033[H";
        tmp += convertToScreenStr(str);
    }
    else
    {
        if (previousframe.size() != str.size())
        {
            // hehehe error here probably
            writeError("previous frame size not equal to current frame size, unexpected");
            // handle
            return; // ??
        }

        int r = 1;
        int c = 1;

        for (std::string::size_type i{0}; i < str.size(); ++i)
        {
            if (str[i] == '\n')
            {
                ++r;
                c = 1;
                continue;
            }
            if(str[i] != previousframe[i])
            {
                struct point res = convertToScrCoordinates(r, c, true);
                for (int x = 0; x < scale; x++)
                {
                    tmp += "\033[" + std::to_string(res.row + x) + ';' + std::to_string(res.col) + 'H';
                    switch(str[i])
                    {
                        case '#':
                            tmp += "\033[48;5;234m";
                            break;
                        case 'o':
                            tmp += "\033[48;5;208m";
                            break;
                        case '=':
                        case '|':
                        case '+':
                        case 'v':
                            tmp += "\033[48;5;88m";
                            break;
                        case '@':
                            tmp += "\033[48;5;21m";
                            break;
                        case '.': // just to be explicit, necessary, unnecessary, not sure
                            tmp += "\033[48;5;244m";
                            break;
                        default:
                            ;;
                    }

                    tmp.append((std::string::size_type)(scale + (int)(scale / 2)), ' ');
                    tmp += "\033[48;5;244m";
                }

            }

            c++; //heheh
        }
        
    }

    write(STDOUT_FILENO, tmp.c_str(), tmp.size());
    previousframe = str;
}

struct point Screen::convertToScrCoordinates(int r, int c, bool is1based)
{
    //1 based
    if (!is1based)
    {
        ++r; ++c;
    }
    struct point result{0,0};
    result.row = (scale * (r - 1)) + 1;
    result.col = ((scale + (int)(scale / 2)) * (c - 1)) + 1;
    return result;
}

Screen::Screen(): winrows{}, wincols{}, originalTerminal{}, previousframe{}, scale{}, logstream{}
{
    enableRawMode();
    enableAltBuffer();
    hideCursor();
    cursorToTopLeft();
    std::pair<int, int> winsize = retrieveWindowSize();
    winrows = winsize.first;
    wincols = winsize.second;

    //calculate scale
    // we know viewport is 20 by 26, hardcoded for now
    // and when scale is applied, its 20 * scale rows, plus 26 * (scale + scale/2)
    // equations are s < wr/20 and s < wc/39, the minimum of those;

    int s1 = (winrows/20);
    int s2 = (wincols/39);
    
    scale = s1 < s2 ? s1 : s2;

    //open log file
    logstream.open("bounce.log");

    //tmp
    //paint whole screen white-ish
    write(STDOUT_FILENO, "\033[48;5;244m", 11);
    clearScreen();
    if (getLog().is_open())
    {
        getLog() << winrows << ' ' << wincols << ' ' << scale << std::endl;
    }
    else
    {
        writeError("error occured in opening log file");
    }
}

Screen::~Screen()
{
    disableRawMode();
    disableAltBuffer();
    restoreCursor();
    logstream.close();
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
