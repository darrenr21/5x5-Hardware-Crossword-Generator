# 5x5 Crossword Puzzle Generator

Algorithm for the 5x5 Hardware Crossword Puzzle — ECE 1896 Senior Design, Team 13
**Author:** Darren Ravichandra | University of Pittsburgh

---

## Overview

This is the crossword puzzle generation algorithm written in C++ for a desktop build (and ported separately to ESP32). It loads a word/clue dictionary from `.txt` files, selects a valid black square pattern, and uses **constraint satisfaction with backtracking** to fill a 5x5 crossword grid. Each run produces a different puzzle thanks to a time-seeded RNG.

---

## How It Works

### 1. Pattern Selection
The generator picks from a set of 6 predefined black square patterns. Before using one, it validates it — all white cells must be connected, there must be at least 2 across and 2 down word slots, every white cell must belong to a slot, and no row or column can have a white run shorter than 3.

### 2. Dictionary Loading
Words are loaded from `.txt` files in the `dictionaries/` folder. Files are named `{language}_{length}.txt` (e.g. `english_3.txt`, `english_4.txt`, `english_5.txt`). Each line is formatted as `WORD,clue text`. The parser validates each entry — wrong length, non-alphabetic characters, and missing commas are all caught and logged.

### 3. Word Index
A `WordIndex` is built from the loaded dictionary. It maps each word length and character position to the list of entries that have a specific letter at that position. This lets `get_candidates()` filter to matching words in one lookup instead of scanning the whole dictionary every time.

### 4. Constraint Satisfaction + Backtracking
This is the core algorithm in `backtrack()`:
- Picks the most constrained unfilled slot using the **MRV heuristic** (minimum remaining values — fewest valid candidates)
- Calls `get_candidates()` to find words that fit the letters already placed by crossing words
- Places a word and recurses
- If no word fits any slot, undoes the last placement and tries the next candidate
- If the step budget (50,000 calls) or time budget (5 seconds) is exceeded, the attempt is abandoned and a new pattern is tried

This budget system prevents the algorithm from getting stuck on pathological cases that would otherwise take minutes to exhaust.

### 5. Output
Once a valid puzzle is found, `print_puzzle()` prints the grid and all clues with answers to stdout.

---

## File Structure

```
├── main.cpp          # entry point — loads dictionary, runs generator, prints result
├── algorithm.h       # data structures and function declarations
├── algorithm.cpp     # puzzle generation logic (backtracking, pattern validation, candidate filtering)
├── dictionary.h      # DictEntry and WordDB type definitions
├── dictionary.cpp    # dictionary loading, parsing, and language detection
└── dictionaries/     # folder containing word/clue .txt files
    ├── english_3.txt
    ├── english_4.txt
    └── english_5.txt
```

---

## How to Run

### Requirements
- C++17 or later
- A compiler with `std::filesystem` support (GCC 8+, Clang 7+, MSVC 2017+)

### Build
```bash
g++ -std=c++17 -O2 -o crossword main.cpp algorithm.cpp dictionary.cpp
```

### Run
```bash
./crossword
```

### Dictionary File Format
Each `.txt` file in `dictionaries/` should follow this format:
```
CAT,A common household pet
DOG,Man's best friend
# lines starting with # are ignored
```
Words are automatically converted to uppercase. Lines with missing commas, wrong lengths, or non-letter characters are skipped and logged as warnings.

### Changing Language
In `main.cpp`, change this line:
```cpp
std::string chosen_language = "english";
```
Any language with complete 3/4/5-letter files in `dictionaries/` will be detected automatically.

---

## Example Output

```
Available languages: english

Generating puzzle in: english
[DICT] Loaded 412 words from english_3.txt
[DICT] Loaded 891 words from english_4.txt
[DICT] Loaded 1203 words from english_5.txt

[ATTEMPT] Attempt 1 failed (steps: 3241), retrying...
[INFO] Puzzle generated (attempt 2/20, steps: 847)
Generation time: 0.38s

5x5 CROSSWORD PUZZLE
Language: english

  +---+---+---+---+---+
  |###|###| C | A | T |
  +---+---+---+---+---+
  |###| A | L | O | E |
  +---+---+---+---+---+
  | B | R | A | I | N |
  +---+---+---+---+---+
  | E | A | S | E |###|
  +---+---+---+---+---+
  | D | E | N |###|###|
  +---+---+---+---+---+

ACROSS:
  1. (R0C2, 3 letters) Feline animal
     Answer: CAT
  ...

DOWN:
  1. (R1C1, 4 letters) Tropical succulent plant
     Answer: ALOE
  ...
```

---

## Technologies Used

| Technology | Purpose |
|---|---|
| C++17 | Core language |
| `std::filesystem` | Dictionary file discovery and validation |
| Constraint Satisfaction + Backtracking | Core puzzle generation algorithm |
| MRV Heuristic | Slot selection to minimize search space |
| WordIndex (positional letter index) | Fast candidate filtering |
| Mersenne Twister (`std::mt19937`) | Seeded RNG for varied puzzle output |
| Step + time budgets | Prevents pathological long runs |

---

## Part of a Larger System

This algorithm is one component of a larger embedded crossword puzzle system built for ECE 1896 Senior Design at the University of Pittsburgh. The full system includes:
- OLED display for rendering the 5x5 letter grid
- LCD panel for displaying clues
- Rotary encoder for letter selection
- Push buttons for clue navigation and menu control
- ESP32 port of this algorithm running on-device via LittleFS
