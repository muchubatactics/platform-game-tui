#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <utility>
#include <string>
#include <sys/ioctl.h>
#include <vector>
#include <string_view>

struct point
{
    point(int x, int y) : row{x}, col{y} {};
    int row;
    int col;
};

enum class CharacterType {
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
        std::vector<Character*> characters{};
        std::string gamestate{};
        int coinsleft{};
        std::string::size_type currentLevelRowLength{};
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
        const std::string convertGameStateToScreenStr(std::string_view);

    private:
        Screen& screenref;
        Game& gameref;
        int scale;
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

ViewPort::ViewPort(Screen& s, Game& g): screenref{s}, gameref{g}, scale{4} // will eventually have to calculate scale from the screen row col size
{}

ViewPort::~ViewPort()
{}

void ViewPort::draw()
{
    const std::string& cur = gameref.getGameState();
    int i = gameref.getCharacterIndexInGameState(gameref.getMainCharacter()); // converts the Player* to Character*
    if (i == -1)
    {
        //something is wrong
        // handle it
        return;
    }

    // let target a 20 rows by 26 cols, for now
    // 6 14 6 for cols 
    // 5 10 5 for rows
    //
    // BE CAREFUL WE ARE USING LONG UNSIGNED INTS HERE, SUBTRACTING FROM ZERO WILL OVERFLOW TO A HUGE INT
    const std::string::size_type vpc = 26;
//    const int vpr = 20;
    std::string::size_type bc = 0;
    const std::string::size_type l = gameref.getCurrentLevelRowLength();
    
    struct point pnt = gameref.getMainCharacter()->getCurrentPoint();
   
    // pnt row col is 1 based, a relic from the tui winsize being 1 based
    if (pnt.col - 1 >= (int)l - 6 - 14)
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

    // when we put movement this will become more complicated;
    std::string buffer{cur.substr(bc, vpc)};
    //reserve?
    for (std::string::size_type i = l; i < cur.size(); i += l + 1)
    {
        if (cur[i] == '\n' && i + 1 < cur.size())
        {
            buffer += '\n';
            buffer += cur.substr(i + 1 + bc, vpc);
        }
    }

    screenref.draw(convertGameStateToScreenStr(buffer));
}

const std::string ViewPort::convertGameStateToScreenStr(std::string_view sv)
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
                                for (int i = 0; i < scale + (int)(scale / 2); ++i) line += " ";
                                line += "\033[48;5;244m";
                                break;
                            case '+':
                                line += "\033[48;5;88m";
                                for (int i = 0; i < scale + (int)(scale / 2); ++i) line += " ";
                                line += "\033[48;5;244m";
                                break;
                            case 'o':
                                for (int i = 0; i < scale + (int)(scale / 2); ++i) line += Coin::getStr();
                                break;
                            case '=':
                            case '|':
                            case 'v':
                                for (int i = 0; i < scale + (int)(scale / 2); ++i) line += Lava::getStr();
                                break;
                            case '@':
                                for (int i = 0; i < scale + (int)(scale / 2); ++i) line += Player::getStr();
                                break;
                            default:
                                for (int i = 0; i < scale + (int)(scale / 2); ++i) line += " ";
                        }
                }
            });
    return buffer;
}

Game::Game()
{
    currentLevel = 2;

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
    //draw the characters; maybe;
    paintCharactersInGameState();
}

std::string::size_type Game::getCurrentLevelRowLength()
{
    return currentLevelRowLength;
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


// given the viewport, should the screen still draw??
// probably not, but we shall see
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
    clearScreen();
    cursorToTopLeft();
    write(STDOUT_FILENO, str.c_str(), str.length());
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
