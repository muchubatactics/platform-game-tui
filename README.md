### a platform tui game
initially intended to be based off the 'bounce' game on old nokia phones
but due to skill-issues, decided to base it off of [this](https:://github.com/muchubatactics/platform-game-js)

is far from complete at the moment, 
has some known issues ( and likely some unknown ones ) ( to me )

will not compile on windows ( probably )

```bash
make
make format # format only the code you've changed
make debug # compile the program in debug mode
```
to compile on linux cli, if you have g++. to format you'll need to have `clang-format-diff` installed

should work on most terminal emulators
( will go into compatibility when base functionality is done )

#### NOTE
because of autorepeat delay on terminal, its hard and sometimes impossible to pass some stages of the game
to reduce the autorepeat delay on your terminal use the following commands

`xset` command works on linux with X11, on console you can use `kdbrate -d 200 -r 20`
else, you might need alternatives

```bash
xset r rate 200 20 #200ms for the autorepeat delay, 20 repeats/sec
```

```bash
./bounce #to run the game
```

```bash
xset r rate #to reset your autorepeat delay settings to the defaults
```
cheers!! ::))
