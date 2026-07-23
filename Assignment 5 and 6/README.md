# Half-Moon Hold 'Em

**Student:** Monet Montgomery  
**Date:** July 22, 2026  
**Language:** C++17

Half-Moon Hold 'Em is a persistent, single-player casino game played against a computer-controlled House opponent. It starts with Texas Hold 'Em, expands the deck to 66 cards, adds three specialty ranks per suit, introduces immediate Joker consequences, and gives each player one dangerous chance to say, “Hit me.”

## The problem it solves

Many beginner card-game projects play only one temporary round and forget everything when the program closes. This project combines an original game with a reusable player-record system. It saves profiles, casino balances, statistics, and match history so a player can leave and continue later.

## Assignment requirement checklist

- **Long-term storage:** `players.csv` is loaded at startup and safely rewritten after updates; `game_history.csv` is an append-only audit log.
- **Add/view/update/delete:** users can create, select, search, rename, and delete player profiles.
- **Classes and objects:** `Card`, `Deck`, `HandEvaluator`, `DataStore`, and `HalfMoonGame` divide responsibilities.
- **Functions:** input checking, CSV parsing, hand comparison, menus, betting, effects, and reporting use focused functions.
- **Decision making and loops:** menus repeat until exit, betting branches on player choices, and the evaluator searches every five-card combination.
- **Input validation:** menu choices are range-checked; names are checked for length and unsafe CSV characters.
- **Searching and sorting:** exact-name search and a balance/wins leaderboard are included.
- **Originality:** the 66-card deck, specialty effects, Joker rule, rotating dealer, and 31-point Hit mechanic make the application unique.
- **Testing:** `--self-test` checks deck construction and important hand-evaluation rules.

## Project structure

```text
HalfMoonHoldEm/
├── src/main.cpp                  Complete C++ source
├── data/players.csv              Persistent player profiles
├── data/game_history.csv         Persistent hand history
├── Makefile                      Build, run, test, and demo commands
├── HalfMoon_Hold_Em_AI_Answer.txt
├── Project_Scope_Of_Work.txt
├── SUBMISSION_LINKS.txt
└── README.md
```

The program creates missing data files automatically. It also supports `--data-dir PATH`, which makes testing possible without changing the real player records.

## Compile and run

### Visual Studio / Developer PowerShell on Windows

```powershell
cl /EHsc /std:c++17 src\main.cpp /Fe:half_moon_holdem.exe
.\half_moon_holdem.exe
```

### g++ on Windows, macOS, or Linux

```bash
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2 src/main.cpp -o half_moon_holdem
./half_moon_holdem
```

### Make commands

```bash
make          # compile
make run      # run normally
make test     # run automated tests
make demo     # run a deterministic automatic hand
```

Run these commands from the `HalfMoonHoldEm` folder so the default `data` folder is found.

## Useful command-line options

```text
--self-test          Run automated checks and exit
--demo               Play one automatic presentation-friendly hand
--seed N             Use a repeatable random seed
--data-dir PATH      Use a different storage directory
--help               Show command syntax
```

For example:

```bash
./half_moon_holdem --demo --seed 20260722 --data-dir demo_data
```

## How the game works

The deck contains 52 standard cards, Duke/Duchess/Reaper in each of the four suits, and two Jokers. A human player faces the House in heads-up Hold 'Em. The dealer rotates after every counted hand, blinds begin the pot, and betting happens before the Flop, after the Flop, after the Turn, and once more after the River.

After the River, either player may Hit and receive one more usable hole card. A player who Hits must build a final five-card poker hand worth no more than 31 points. Then privately held specialty cards resolve:

- **Duke:** closes the final heads-up betting round.
- **Duchess:** reverses order and makes each answer cost twice the wager.
- **Reaper:** forces the opponent to draw two trap cards; a Joker eliminates the target.

A Joker dealt or drawn privately eliminates that hand. A Joker in the community kills the entire hand and refunds the human player's committed bets.

## Digital table rulings

Two paper rules needed precise definitions before they could become code:

1. Duke, Duchess, and Reaper are evaluated as Jack, Queen, and King equivalents respectively. Their separate specialty priority—Reaper, Duchess, then Duke—breaks ties between equal hand categories before ordinary rank comparison.
2. Only privately held specialty cards activate. A specialty card in the community still helps every player's poker hand, but it does not activate once for every player.

These rulings appear inside the program's Rules menu so the application never surprises the player.

## Persistent data

`players.csv` stores one current record per player:

```text
name,balance,wins,losses,ties,folds,joker_losses,hits,busts,games_played
```

`game_history.csv` keeps a timestamped log:

```text
timestamp,player,result,wagered,payout,balance_after,detail
```

Player data is written to a temporary file and then renamed into place. This reduces the chance of leaving half a profile file if saving is interrupted.

## Testing performed

The project was compiled with:

```text
g++ -std=c++17 -Wall -Wextra -Wpedantic -O2
```

The automated suite passed all of these checks:

- 66 total cards
- 2 Jokers
- 12 specialty cards
- every suited card unique
- Royal Flush recognition
- Ace changes from 11 to 1 when required by the 31-point cap

A deterministic full hand was also played through all stages, won at showdown, saved to CSV, and displayed again through the recent-history screen.

## Suggested future improvements

- Add two to six local human players.
- Give the House adjustable difficulty and bluffing personalities.
- Add an optional graphical interface with SFML or Qt.
- Train or load an AI betting model and compare it with the rule-based House.
- Encrypt or sign tournament data files to discourage manual score editing.

## GitHub and Google Slides submission

1. Create a GitHub repository named `HalfMoonHoldEm`.
2. Upload the contents of this folder, keeping `src` and `data` labeled.
3. Import `Half_Moon_Hold_Em_Presentation.pptx` into Google Slides.
4. Set the sharing permissions required by the instructor.
5. Paste both share links into `SUBMISSION_LINKS.txt` and upload that TXT to Canvas.

