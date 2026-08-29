#include <sys/poll.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <utility>
#include <string>
#include <sys/ioctl.h>
#include <vector>
#include <string_view>
#include <fstream>
#include <chrono>

// for setting up std::cerr as the way to write into my logFile
class Logger
{
    public:
        Logger(const char*);
        ~Logger();

        //disable copying
        Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

    private:
        std::ofstream logFile;
        std::streambuf* oldStream;
};

Logger::Logger(const char* fn): logFile{}, oldStream{nullptr}
{
    logFile.open(fn);
    if (logFile.is_open())
    {
        oldStream = std::cerr.rdbuf(logFile.rdbuf());
    }
}

Logger::~Logger()
{
    if (oldStream)
    {
        std::cerr.rdbuf(oldStream);
    }
}

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
        const struct point startingPoint;
        struct point currentPoint;
        const struct point endingPoint;

    public:
        Character(struct point, struct point);
        Character(struct point);
        virtual ~Character() = default;
        virtual struct point getNextPoint() = 0;
        struct point getCurrentPoint() const;
        void resetPoint();
        virtual const std::string& getStrRepresentation() = 0;
        virtual CharacterType getType() = 0;
        virtual void onWallCollision();
        virtual void onLavaCollision();
        void setCurrentPoint(int, int);
        const struct point getStartingPoint() const;
};

Character::Character(struct point start, struct point end):
    startingPoint{start}, currentPoint{start}, endingPoint{end}
{
    // reserve str?
    // do everything here
}

Character::Character(struct point start):
    Character(start, {-1, -1})
{};

void Character::onWallCollision()
{};

void Character::onLavaCollision()
{};

void Character::resetPoint()
{
    currentPoint = startingPoint;
}

const struct point Character::getStartingPoint() const
{
    return startingPoint;
}

struct point Character::getCurrentPoint() const
{
    return currentPoint;
}

/*
* deprecated
*/
std::string deprecatedCreateCharacterStr(std::string color, std::string str, int scale)
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
    currentPoint.row = row;
    currentPoint.col = col;
}

class Player: public Character
{
    private:
        int horizontalForce{};
        std::chrono::steady_clock::time_point jumpTime{};
        const std::chrono::milliseconds jumpDuration{500};
        const std::chrono::milliseconds jumpStep{100};

        static inline const std::string str{"\033[48;5;21m \033[48;5;244m"};
        static inline CharacterType ctype = CharacterType::Player;

    public:
        Player(struct point start);
        void consumeKeyPress(char, bool);
        bool isJumping();
        int jumpBy();
        void stopJump();

        // overides
        struct point getNextPoint() override;
        const std::string& getStrRepresentation() override;
        CharacterType getType() override { return ctype; }
        void onWallCollision() override;

        //static
        static const std::string& getStr();
};

Player::Player(struct point start):
    Character{start}
{
}

bool Player::isJumping()
{
    if (jumpTime == std::chrono::steady_clock::time_point{}) return false;
    if ((std::chrono::steady_clock::now() - jumpTime) >= jumpDuration) return false;
    return true;
}

void Player::stopJump()
{
    jumpTime = std::chrono::steady_clock::time_point{};
}

void Player::onWallCollision()
{
    stopJump();
}

const std::string& Player::getStrRepresentation()
{
    return str;
}

const std::string& Player::getStr()
{
    return str;
}

int Player::jumpBy()
{
    static std::chrono::steady_clock::time_point lastTick{std::chrono::steady_clock::now()};
    std::chrono::steady_clock::time_point tnow{std::chrono::steady_clock::now()};

    int value = 0;
    if (tnow - lastTick > jumpStep)
    {
        value = 1;
        lastTick = tnow;
    }

    if (isJumping()) value *= -1;

    return value;
}

struct point Player::getNextPoint()
{
    // naive impl
    struct point cur = getCurrentPoint();

    cur.row += jumpBy();

    if (horizontalForce)
    {
        if (horizontalForce > 0)
        {
            horizontalForce--;
            ++cur.col;
        } else {
            horizontalForce++;
            --cur.col;
        }
    }
    return cur;
}

void Player::consumeKeyPress(char c, bool canJump)
{
    switch(c)
    {
        case 'w':
            if (canJump) jumpTime = std::chrono::steady_clock::now();
            break;
        case 'a':
            horizontalForce--;
            break;
        case 's':
            break;
        case 'd':
            horizontalForce++;
            break;
        default:
            ;;
    }
}

class Lava: public Character
{
    private:
        int xSpeed;
        int ySpeed;
        bool resets;
        static inline const std::string str{"\033[48;5;88m \033[48;5;244m"};
        static inline const CharacterType ctype = CharacterType::Lava;

    public:
        Lava(struct point s, int v, int h, bool resets);

        //overides
        struct point getNextPoint() override;
        const std::string& getStrRepresentation() override;
        CharacterType getType() override { return ctype; }
        void onWallCollision() override;
        void onLavaCollision() override;

        //static
        static const std::string& getStr();
};

Lava::Lava(struct point start, int vertical, int horizontal, bool resets):
    Character{start}, xSpeed{horizontal}, ySpeed{vertical}, resets{resets}
{
}

void Lava::onWallCollision()
{
    if (resets)
    {
        const struct point sp = getStartingPoint();
        setCurrentPoint(sp.row, sp.col);
    }
    else
    {
        if (xSpeed) xSpeed *= -1;
        if (ySpeed) ySpeed *= -1;

        // to ensure it bounces immediately, not on next frame ?? good impl or bad impl ? not sure yet,
        struct point dest = getNextPoint();
        setCurrentPoint(dest.row, dest.col);
    }
}

void Lava::onLavaCollision()
{
    // treat it as wall
    onWallCollision();
}

struct point Lava::getNextPoint()
{
    struct point cur = getCurrentPoint();
    if (xSpeed)
    {
        cur.col -= xSpeed;
    }
    if (ySpeed)
    {
        cur.row += ySpeed;
    }
    return cur;
}

const std::string& Lava::getStrRepresentation()
{
    return str;
}

const std::string& Lava::getStr()
{
    return str;
}

class Coin: public Character
{
    private:
        bool consumed{false};
        static inline const std::string str{"\033[48;5;208m \033[48;5;244m"};
        static inline const CharacterType ctype = CharacterType::Coin;

    public:
        Coin(struct point start);
        void setConsumed(bool);
        bool isConsumed();

        //overrides ?
        struct point getNextPoint() override;
        const std::string& getStrRepresentation() override;
        CharacterType getType() override { return ctype; }

        //static
        static const std::string& getStr();
};

Coin::Coin(struct point start):
    Character{start}
{
}

bool Coin::isConsumed()
{
    return consumed;
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
    this->setCurrentPoint(-1, -1);
}

struct point Coin::getNextPoint()
{
    if(consumed)
    {
        return {-1, -1};
    }
    else return getCurrentPoint();
}

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

        //TODO::handle when screen size changes during game
        std::pair<int, int> getWindowSize();
        std::pair<int, int> retrieveWindowSize();

        const std::string convertToScreenStr(std::string_view);
        void writeError(std::string_view);
        struct point convertToScrCoordinates(int, int, bool);

    private:
        int winrows;
        int wincols;
        struct termios originalTerminal;
        std::string previousFrame;
        int scale;
};

class Game
{
    private:
        std::vector<std::string_view> gameLevels{};
        int currentLevel{};
        std::vector<Character*> gameCharacters{};
        std::string gameState{};
        int coinsRemaining{};
        int livesRemaining{};
        std::string::size_type currentLevelWidth{};
        std::string::size_type currentLevelHeight{};
        Player* mainCharacter{nullptr};
        
        bool playerDead{false}; 

        //time
        std::chrono::steady_clock::time_point lastTick{};
        const std::chrono::milliseconds tickDuration{200};

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
        int getCharacterIndexInGameState(int row, int col);
        std::string::size_type getCurrentLevelRowLength();
        std::string::size_type getCurrentLevelHeight();

        void initLevel(int level);
        void progressLevel();
        void restartLevel();
        void endGame();
        void restartGame();
        void setInitialState();

        bool isPlayerDead();
        bool isLivesDepleted();
        bool isGameCompleted();
        bool isLevelComplete();

        void setPlayerDead(bool);
        bool canCharacterJump();

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
        void calculateTopLeft(std::string::size_type, std::string::size_type, std::string::size_type, std::string::size_type, bool);
        void updateTopLeft(std::string::size_type, std::string::size_type, std::string::size_type, std::string::size_type);

    private:
        Screen& screenRef;
        Game& gameRef;
        std::string::size_type top{0};
        std::string::size_type left{0};
        // this shouldReset variable is under utilized, impl was a bit complicated,
        // it should be set whenever a we progress a level, but where?
        // at the moment, we rely on updateTopLeft to realize that the main character is not in viewport
        // then to call calculateTopLeft to update the view port
        bool shouldReset{true};

        const std::string playerDeadScreen{"\
..........................\n\
..........................\n\
..........................\n\
..........................\n\
..........................\n\
.......#.#.###.#.#........\n\
.......#.#.#.#.#.#........\n\
........#..#.#.#.#........\n\
........#..###.###........\n\
..........................\n\
.....##..###.###.##.......\n\
.....#.#..#..#...#.#......\n\
.....#.#..#..##..#.#......\n\
.....##..###.###.##.......\n\
..........................\n\
..........................\n\
..........................\n\
..........................\n\
..........................\n\
..........................\n"};

        const std::string livesDepletedScreen{"\
..........................\n\
.....###.###.#.#.###......\n\
.....#...#.#.###.#........\n\
.....#.#.###.###.##.......\n\
.....###.#.#.#.#.###......\n\
..........................\n\
.....###.#.#.###.###......\n\
.....#.#.#.#.#...#.#......\n\
.....#.#.#.#.##..###......\n\
.....###..#..###.#.#......\n\
..........................\n\
...#...###.#.#.###.###....\n\
...#....#..#.#.#...#......\n\
...#....#..#.#.##..###....\n\
...###.###..#..###...#....\n\
..........................\n\
.....##..###.###.###......\n\
.....#.#.#.#.#.#.#........\n\
.....#.#.#.#.#.#.##.......\n\
.....##..###.#.#.###......\n"};

        const std::string allLevelsCompletedScreen{"\
..........................\n\
.....###.###.###.###......\n\
.....#.#..#..#...#........\n\
.....#.#..#..#...##.......\n\
.....#.#.###.###.###......\n\
..........................\n\
.......###.#...#..........\n\
.......#.#.#...#..........\n\
.......###.#...#..........\n\
.......#.#.###.###........\n\
..........................\n\
.#...###.#.#.###.#...###..\n\
.#...#...#.#.#...#...#....\n\
.#...##..#.#.##..#...###..\n\
.###.###..#..###.###...#..\n\
..........................\n\
.....##..###.###.###......\n\
.....#.#.#.#.#.#.#........\n\
.....#.#.#.#.#.#.##.......\n\
.....##..###.#.#.###......\n"};

        const std::string levelCompleteScreen{"\
..........................\n\
..........................\n\
..........................\n\
..........................\n\
..........................\n\
...#...###.#.#.###.#......\n\
...#...#...#.#.#...#......\n\
...#...##..#.#.##..#......\n\
...###.###..#..###.###....\n\
..........................\n\
.....##..###.###.###......\n\
.....#.#.#.#.#.#.#........\n\
.....#.#.#.#.#.#.##.......\n\
.....##..###.#.#.###......\n\
..........................\n\
..........................\n\
..........................\n\
..........................\n\
..........................\n\
..........................\n"};

};

int main()
{
    Logger log{"bounce.log"};
    Screen S;
    Game G;
    ViewPort vp{S, G};
    vp.draw();
    char c = 0;

    std::vector<std::chrono::steady_clock::duration> consumetime{};
    std::vector<std::chrono::steady_clock::duration> updatestatetime{};
    std::vector<std::chrono::steady_clock::duration> drawtime{};
    for (;;)
    {
        struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
        int ready = poll(&pfd, 1, 20);

        if (ready > 0 && (pfd.events & POLLIN))
        {
            while(read(STDIN_FILENO, &c, 1) == 1)
            {
                if (c == 'q') break;
                auto t1 = std::chrono::steady_clock::now();
                G.consumeKey(c);
                auto t2 = std::chrono::steady_clock::now();
                consumetime.push_back(t2 - t1);
            }
        }
        
        if (c == 'q') break;

        auto t1 = std::chrono::steady_clock::now();
        G.updateGameState();
        auto t2 = std::chrono::steady_clock::now();
        updatestatetime.push_back(t2 - t1);


        auto t3 = std::chrono::steady_clock::now();
        vp.draw();
        auto t4 = std::chrono::steady_clock::now();
        drawtime.push_back(t4 - t3);
    }

    std::chrono::steady_clock::duration total{};
    for (auto t : consumetime) total += t;
    if(consumetime.size()) std::cerr << "average time to run, Game::consumeKey(char) in milliseconds: " << std::chrono::duration<double, std::milli>(total / consumetime.size()).count() << std::endl;
    total = std::chrono::steady_clock::duration{};

    for (auto t : updatestatetime) total += t;
    std::cerr << "average time to run, Game::updateGameState() in milliseconds: " << std::chrono::duration<double, std::milli>(total / updatestatetime.size()).count() << std::endl;
    total = std::chrono::steady_clock::duration{};

    for (auto t : drawtime) total += t;
    std::cerr << "average time to run, ViewPort::draw() in milliseconds: " << std::chrono::duration<double, std::milli>(total / drawtime.size()).count() << std::endl;
    total = std::chrono::steady_clock::duration{};

    return 0;
}

ViewPort::ViewPort(Screen& s, Game& g): screenRef{s}, gameRef{g}
{}

ViewPort::~ViewPort()
{}

void ViewPort::updateTopLeft(std::string::size_type l, std::string::size_type h, std::string::size_type vpc, std::string::size_type vpr)
{
    if (!gameRef.getMainCharacter())
    {
        std::cerr << "main character ptr is null\n";
        return;
    }
    struct point pnt = gameRef.getMainCharacter()->getCurrentPoint();

    if (
            (pnt.col - 1)> (int)(left + vpc) ||
            (pnt.col - 1) < (int)left ||
            (pnt.row - 1) > (int)(top + vpr) ||
            (pnt.row - 1) < (int)top
       )
    {
        std::cerr << "main character doesnt lie in view port, attempting to recalculate view port\n";
        calculateTopLeft(l,h,vpc,vpr,false);
        return;
    }

    int tmp = (int)left;

    if (pnt.col - 1 > tmp + 6 + 14)
    {
        tmp += (pnt.col - 1) - (tmp + 6 + 14);
        if (tmp > (int)(l - vpc)) tmp = (int)(l - vpc);
        left = (std::string::size_type)tmp;
    }
    else if (pnt.col - 1 - tmp < 6)
    {
        tmp -= (tmp + 6) - (pnt.col - 1);
        if (tmp < 0) tmp = 0;
        left = (std::string::size_type)tmp;
    }

    tmp = (int)top;

    if (pnt.row - 1 > tmp + 7 + 6)
    {
        tmp += (pnt.row - 1) - (tmp + 7 + 6);
        if (tmp > (int)(h - vpr)) tmp = (int)(h - vpr);
        top = (std::string::size_type)tmp;
    }
    else if (pnt.row - 1 - tmp < 7)
    {
        tmp -= (tmp + 7) - (pnt.row - 1);
        if (tmp < 0) tmp = 0;
        top = (std::string::size_type)tmp;
    }
}


void ViewPort::calculateTopLeft(std::string::size_type l, std::string::size_type h, std::string::size_type vpc, std::string::size_type vpr, bool isScreen)
{
    // let target a 20 rows by 26 cols, for now
    // 6 14 6 for cols 
    // 7 6 7 for rows
    struct point pnt{1, 1};
    if (!isScreen) pnt = gameRef.getMainCharacter()->getCurrentPoint();
   
    // pnt row col is 1 based, a relic from the tui winsize being 1 based
    if (vpc >= l)
    {
        left = 0;
    }
    else if (pnt.col - 1 >= (int)l - 6 - 14)
    {
        left = l - vpc;
    }
    else if (pnt.col - 1 <= 20)
    {
        left = 0;
    }
    else
    {
        // can either be on 6 or on 6 + 14, depending on previous position
        // but lets initialize it to 6 for now
        left = (std::string::size_type)pnt.col - 1 - 6;
    }

    if (vpr >= h)
    {
        top = 0;
    }
    else if (pnt.row - 1 >= (int)h - 7 - 6)
    {
        top = h - vpr;
    }
    else if (pnt.row - 1 <= 13)
    {
        top = 0;
    }
    else
    {
        top = (std::string::size_type)pnt.row - 1 - 7;
    }
}

void ViewPort::draw()
{
    const std::string* pstr = nullptr;
    bool isScreen{true};
    if (gameRef.isGameCompleted())
    {
        pstr = &allLevelsCompletedScreen;
    }
    else if (gameRef.isLivesDepleted())
    {
        pstr = &livesDepletedScreen;
    }
    else if (gameRef.isPlayerDead())
    {
        pstr = &playerDeadScreen;
    }
    else if (gameRef.isLevelComplete())
    {
        pstr = &levelCompleteScreen;
    }
    else
    {
        isScreen = false;
        pstr = &(gameRef.getGameState());
    }

    if (isScreen)
    {
        shouldReset = true;
    }

    const std::string& cur = *pstr;
    const std::string::size_type l = isScreen ? 26ul : gameRef.getCurrentLevelRowLength(); // if this is n, it means theres n + 1 gameCharacters in the str per row, the + 1 being the \n character
    const std::string::size_type h = isScreen ? 20ul : gameRef.getCurrentLevelHeight(); // if this is n, it means theres n lines;

    const std::string::size_type vpc = std::min(l, 26ul);
    const std::string::size_type vpr = std::min(h, 20ul);
    if (shouldReset)
    {
        calculateTopLeft(l, h, vpc, vpr, isScreen);
        shouldReset = false;
    }
    else updateTopLeft(l, h, vpc, vpr);

    std::string sv{cur.substr(top * (l + 1))}; // why wont string_view work here ?
    std::string buffer{sv.substr(left, vpc)};
    //reserve?
    //
    //this i is supposed to point to the first \n char after the line we just initialized the buffer with
    std::string::size_type i{l};

    for (std::string::size_type rr = 1; i < sv.size() && rr < vpr; i += l + 1, ++rr)
    {
        if (sv[i] == '\n' && i + 1 < sv.size())
        {
            buffer += '\n';
            buffer += sv.substr(i + 1 + left, vpc);
        }
        else
        {
            ;;
        }
    }

    screenRef.draw(buffer);
}

void Game::initLevel(int level)
{
    if (level < 1)
    {
        std::cerr << "invalid level\n";
        return;
    }

    std::string buffer{};
    //reserve buffer ?
    
    std::string_view thislevel = gameLevels[(std::string_view::size_type)level - 1];
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
                                gameCharacters.push_back(c);
                                buffer += '.';
                                break;
                            }
                        case '=':
                            {
                                Lava* c = new Lava({rown, coln}, 0, 1, false);
                                gameCharacters.push_back(c);
                                buffer += '.';
                                break;
                            }
                        case '|':
                            {
                                Lava* c = new Lava({rown, coln}, 1, 0, false);
                                gameCharacters.push_back(c);
                                buffer += '.';
                                break;
                            }
                        case 'v':
                            {
                                Lava* c = new Lava({rown, coln}, 1, 0, true);
                                gameCharacters.push_back(c);
                                buffer += '.';
                                break;
                            }
                        case '@':
                            {
                                Player* c = new Player({rown, coln});
                                mainCharacter = c;
                                gameCharacters.push_back(c);
                                buffer += '.';
                                break;
                            }
                        default:
                            buffer += '.';
                    }
                    ++coln;
                }
            });

    coinsRemaining = coinnumber;
    gameState = buffer;
    currentLevelWidth = thislevel.find('\n');
    currentLevelHeight = thislevel.size() / (currentLevelWidth + 1);
    std::cerr << "width, height = " << currentLevelWidth << ' ' << currentLevelHeight << '\n';
    paintCharactersInGameState();
}

bool Game::canCharacterJump()
{
    int idx = getCharacterIndexInGameState(mainCharacter); // upcasting Player* to Character*
    if (idx < 0) return false;
    std::string::size_type indexBelow = (std::string::size_type)idx + currentLevelWidth + 1;
    if (indexBelow >= gameLevels[(std::string::size_type)(currentLevel - 1)].size())
    {
        std::cerr << "error in canCharacterJump(), index below greater than level length\n";
        return false;
    }

    return gameLevels[(std::string::size_type)(currentLevel - 1)][indexBelow] == '#';
}

void Game::restartLevel()
{
    setPlayerDead(false);
    currentLevel--;
    progressLevel();
    // hacky ?? implications ??
}

void Game::endGame()
{
    // free mem for old gameCharacters
    for (Character* ptr : gameCharacters)
    {
        delete ptr;
    }

    mainCharacter = nullptr; // no free because its part of the gameCharacters vector, already freed
    gameCharacters = {};
}

void Game::progressLevel()
{
    endGame();
    setPlayerDead(false);
    int newlevel = currentLevel + 1;
    if ((std::vector<std::string_view>::size_type)newlevel > gameLevels.size())
    {
        // game is done, handle
        std::cerr << "all the gameLevels are completed\n";
        return;
    }

    currentLevel = newlevel;
    initLevel(currentLevel);
}

void Game::setInitialState()
{
    currentLevel = 1;
    livesRemaining = 3;
    playerDead = false;
    lastTick = std::chrono::steady_clock::time_point{};
}

void Game::restartGame()
{
    endGame();
    setInitialState();
    initLevel(currentLevel);
}

bool Game::isPlayerDead()
{
    return playerDead;
}

void Game::setPlayerDead(bool val)
{
    playerDead = val;
}

bool Game::isLivesDepleted()
{
    return livesRemaining <= 0;
}

bool Game::isGameCompleted()
{
    return (std::string_view::size_type)currentLevel == gameLevels.size() && coinsRemaining <= 0 && !playerDead;
}

bool Game::isLevelComplete()
{
    return coinsRemaining <= 0;
}

Game::Game()
{
// braces to easily collapse this
{
    gameLevels.push_back(std::string_view{"\
......................\n\
..#................#..\n\
..#..............=.#..\n\
..#.........o.o....#..\n\
..#.@......#####...#..\n\
..#####............#..\n\
......#++++++++++++#..\n\
......##############..\n\
......................\n"});

    gameLevels.push_back(std::string_view{"\
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

    gameLevels.push_back(std::string_view{"\
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

    gameLevels.push_back(std::string_view{"\
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

    gameLevels.push_back(std::string_view{"\
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

    gameLevels.push_back(std::string_view{"\
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

    setInitialState();
    initLevel(currentLevel);
}

std::string::size_type Game::getCurrentLevelRowLength()
{
    return currentLevelWidth;
}

std::string::size_type Game::getCurrentLevelHeight()
{
    return currentLevelHeight;
}

const Player* Game::getMainCharacter()
{
    return mainCharacter;
}

const std::string& Game::getGameState()
{
    return gameState;
}

void Game::removeCharactersFromGameState()
{
    for (std::string::size_type i = 0; i < gameState.size(); ++i)
    {
        switch(gameState[i])
        {
            case '@':
            case '=':
            case 'o':
                gameState[i] = '.';
            default:
                ;;
        }
    }
}

int Game::getCharacterIndexInGameState(const Character* cptr)
{
    struct point pos = cptr->getCurrentPoint();
    return getCharacterIndexInGameState(pos.row, pos.col);
}

int Game::getCharacterIndexInGameState(int row, int col)
{
    // we are taking advantage of all lines being of equal length;
    // reducing it to (n + 1) * (r - 1) + (c - 1)
    std::string::size_type n = currentLevelWidth;
    struct point pos = {row, col}; 

    if (!pos.row || !pos.col)
    {
        //error handle
        std::cerr << "error in getCharacterIndexInGameState, invalid point: " << pos.row << ' ' << pos.col << '\n';
        return -1;
    }
    if (pos.row == -1 && pos.col == -1)
    {
        //okay
        return -1;
    }
    if ((pos.row < 0) != (pos.col < 0))
    {
        // error handle
        std::cerr << "error in getCharacterIndexInGameState, invalid point: " << pos.row << ' ' << pos.col << '\n';
        return -1;
    }
    return ((static_cast<int>(n) + 1) * (pos.row - 1)) + (pos.col - 1);
}

void Game::paintCharactersInGameState()
{
    // remove existing gameCharacters
    removeCharactersFromGameState();
    for (Character* ptr : gameCharacters)
    {
        int i = getCharacterIndexInGameState(ptr);
        if (i == -1)
        {
            continue; // invalid index
        }

        std::string::size_type index = (std::string::size_type)(i);
        if (index >= gameState.length())
        {
            std::cerr << "error in paintCharactersInGameState() fn, index greater than gameState length, index: " << index << " length: " << gameState.length() << '\n';
        } // BIG ERROR, handle it
        else
        {
            switch(ptr->getType())
            {
                case CharacterType::Lava:
                    gameState[index] = '=';
                    break;
                case CharacterType::Player:
                    gameState[index] = '@';
                    break;
                case CharacterType::Coin:
                    gameState[index] = 'o';
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
    const std::chrono::time_point tnow = std::chrono::steady_clock::now();
    if (lastTick == std::chrono::steady_clock::time_point{}) // not initialized
    {
        lastTick = tnow;
    }
    bool shouldUpdateLava = (tnow - lastTick) >= tickDuration;

    if (shouldUpdateLava) lastTick = tnow;

    for (Character* ptr : gameCharacters)
    {
        struct point dest = ptr->getNextPoint();
        CharacterType ptrtype = ptr->getType();

        // cancel character trying to go down, if its standing on wall;
        if (ptrtype == CharacterType::Player)
        {
            struct point cur = ptr->getCurrentPoint();
            int belowIndex =  getCharacterIndexInGameState(cur.row + 1, cur.col);
            if (belowIndex >= 0 && (std::string::size_type)belowIndex <= gameState.size())
            {
                if (gameState[(std::string::size_type)belowIndex] == '#' && dest.row > cur.row) dest.row = cur.row;
                //ptr->onWallCollision();
            }
        }

        if (ptrtype == CharacterType::Coin)
        {
            // cant do anything here because the main character could be updated after the coins, then we would miss consuming them
        }
        else
        {
            if (ptrtype == CharacterType::Lava && !shouldUpdateLava)
            {
                // do not update lava if tick hasnt passed
                continue;
            }
            if (dest.row == -1 && dest.col == -1)
            {
                std::cerr << "invalid dest, in updateGameState Fn " << dest.row << ' ' << dest.col << '\n';
                continue;
                // need to see when this can happen
            }
            // bound checks, remember the dest is 1 based
            if (dest.row < 1 || dest.row > (int)currentLevelHeight)
            {
                std::cerr << "out of bounds dest, in updateGameState Fn " << dest.row << ' ' << dest.col << '\n';
                continue;
            }
            if (dest.col < 1 || dest.col > (int)currentLevelWidth)
            {
                std::cerr << "out of bounds dest, in updateGameState Fn " << dest.row << ' ' << dest.col << '\n';
                continue;
            }

            int destIndex = getCharacterIndexInGameState(dest.row, dest.col);

            if (destIndex == -1)
            {
                std::cerr << "invalid dest index in updateGameState fn " << destIndex << '\n';
                continue;
            }

            if (gameState[(std::string::size_type)destIndex] == '#') //collision with wall
            {
                if (ptrtype == CharacterType::Player)
                {
                    // are we jumping dest.row < cur.row ? if not, we know the wall is left or right, we just stop where we are
                    // otherwise, we check if theres a wall in dest.col, if no wall, move there
                    // check if theres wall in dest.row, if not move there, if there is, stop jump;
                    struct point cur = ptr->getCurrentPoint();
                    if (dest.row < cur.row)
                    {
                        int destColIndex = getCharacterIndexInGameState(cur.row, dest.col);
                        if (destColIndex >= 0 && (std::string::size_type)destColIndex < gameState.size() && gameState[(std::string::size_type)destColIndex] != '#')
                        {
                            ptr->setCurrentPoint(cur.row, dest.col);
                        }

                        cur = ptr->getCurrentPoint();
                        int destRowIndex = getCharacterIndexInGameState(dest.row, cur.col);
                        if (destRowIndex >= 0 && (std::string::size_type)destRowIndex < gameState.size() && gameState[(std::string::size_type)destRowIndex] != '#')
                        {
                            struct point tmp = ptr->getCurrentPoint();
                            ptr->setCurrentPoint(dest.row, tmp.col);
                        }
                        else if (destRowIndex >= 0 && (std::string::size_type)destRowIndex < gameState.size() && gameState[(std::string::size_type)destRowIndex] == '#')
                        {
                            ptr->onWallCollision(); // to stop jump timing stuff
                            cur = ptr->getCurrentPoint();
                            int newRowIndex = getCharacterIndexInGameState(cur.row + 1, cur.col);
                            if (newRowIndex >= 0 && (std::string::size_type)newRowIndex < gameState.size() && gameState[(std::string::size_type)newRowIndex] != '#')
                            {
                                ptr->setCurrentPoint(cur.row + 1, cur.col);
                            }
                        }

                    }
                }
                else ptr->onWallCollision();

                continue;
            }

            if (
                    (ptrtype == CharacterType::Player &&
                    (gameState[(std::string::size_type)destIndex] == '+' || gameState[(std::string::size_type)destIndex] == '='))
                    ||
                    (ptrtype == CharacterType::Lava && gameState[(std::string::size_type)destIndex] == '@')
               ) // player collision with static or dynamic lava or dynamic lava collision with player
            {
                livesRemaining--;
                setPlayerDead(true);
            }

            if (ptrtype == CharacterType::Lava && gameState[(std::string::size_type)destIndex] == '+') // dynamic lava collision with static lava
            {
                ptr->onLavaCollision();
                continue;
            }

            if (gameState[(std::string::size_type)destIndex] == 'o' && ptrtype == CharacterType::Player) // player collision with coin
            {
                // find coin
                // consume it
                Coin* cptr = nullptr;
                for (Character* ptr: gameCharacters)
                {
                    struct point cpos = ptr->getCurrentPoint();
                    if (cpos.row == dest.row && cpos.col == dest.col && ptr->getType() == CharacterType::Coin)
                    {
                        cptr = static_cast<Coin*>(ptr);
                    }
                }

                if(!cptr)
                {
                    std::cerr << "failed to find the coin that the player is colliding with\n";
                    // note: we used static cast, because we are quite sure its a coin
                }
                else
                {
                    cptr->setConsumed(true);
                    coinsRemaining--;
                    if (coinsRemaining < 0) std::cerr << "invalid coinsRemaining number " << coinsRemaining << '\n';
                }
            }
            //rest later
            ptr->setCurrentPoint(dest.row, dest.col);
            if (isPlayerDead() || coinsRemaining <= 0)
            {
                endGame();
                return;
            }
        }
    }
    paintCharactersInGameState();
}

Game::~Game()
{
    for (Character* c : gameCharacters)
    {
        delete c;
    }
}

void Game::consumeKey(char c)
{
    if ((int)c == 13) // enter key
    {
        if (isGameCompleted() || isLivesDepleted())
        {
            restartGame();
        }
        else if (isLevelComplete())
        {
            progressLevel();
        }
        else if (isPlayerDead())
        {
            setPlayerDead(false);
            restartLevel();
        }
    }
    else if (mainCharacter) mainCharacter->consumeKeyPress(c, canCharacterJump());
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
    moveCursor(winrows, 0);
    std::ignore = write(STDOUT_FILENO, str.data(), str.size());
}

void Screen::draw(const std::string& str)
{
// new impl
    std::string tmp{};
    if (previousFrame.empty())
    {
        tmp += "\033[H";
        tmp += convertToScreenStr(str);
    }
    else if (previousFrame.size() != str.size())
    {
        std::cerr << "previous frame size not equal to current frame size, mostly happens when level changes, did level change ?\n";
        clearScreen();
        tmp += "\033[H";
        tmp += convertToScreenStr(str);
    }
    else
    {

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
            if(str[i] != previousFrame[i])
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

    std::ignore = write(STDOUT_FILENO, tmp.c_str(), tmp.size());
    previousFrame = str;
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

Screen::Screen(): winrows{}, wincols{}, originalTerminal{}, previousFrame{}, scale{}
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

    //tmp
    //paint whole screen white-ish
    std::ignore = write(STDOUT_FILENO, "\033[48;5;244m", 11);
    clearScreen();

    std::cerr <<"window rows and cols: " << winrows << ' ' << wincols << ' ' << scale << std::endl;
}

Screen::~Screen()
{
    disableRawMode();
    disableAltBuffer();
    restoreCursor();
}

void Screen::clearScreen()
{
    std::ignore = write(STDOUT_FILENO, "\033[2J", 4);
    // check if 4 chars are written to error handle
}

void Screen::hideCursor()
{
    std::ignore = write(STDOUT_FILENO, "\033[?25l", 6);
    // handle error
}

void Screen::restoreCursor()
{
    std::ignore = write(STDOUT_FILENO, "\033[?25h", 6);
    // handle error
}

/*
 * x and y are 1 based
 */
void Screen::moveCursor(int row, int col)
{
    std::string str{"\033["};
    str += std::to_string(row) + ';' + std::to_string(col) + 'H';
    std::ignore = write(STDOUT_FILENO, str.c_str(), str.length());
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
    raw.c_cc[VMIN] = 0; //minimum gameCharacters before read return
    raw.c_cc[VTIME] = 0; //time after which read returns if no input, 1 means 100ms

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return false;
    return true;
}

bool Screen::disableRawMode()
{
    return tcsetattr(STDIN_FILENO, TCSAFLUSH, &this->originalTerminal) != -1;
}

void Screen::enableAltBuffer()
{
    std::ignore = write(STDOUT_FILENO, "\033[?1049h", 8);
    // handle error
}

void Screen::disableAltBuffer()
{
    std::ignore = write(STDOUT_FILENO, "\033[?1049l", 8);
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
