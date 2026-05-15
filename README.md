# 5x5 Hardware Crossword Generator

Algorithm for the 5x5 Hardware Crossword Puzzle — ECE 1896 Senior Design, Team 13  
**Author:** Darren Ravichandra | University of Pittsburgh

---

## Overview

This is the crossword puzzle generation algorithm written in C++ for an ESP32 microcontroller. It runs on boot, loads a dictionary from the onboard LittleFS filesystem, selects a valid black square pattern, and uses **constraint satisfaction with backtracking** to fill a 5x5 crossword grid with real words and clues. The generated puzzle is then displayed on an OLED screen with an LCD panel for clues, a rotary encoder for letter selection, and push buttons for navigation.

---

## How It Works

### 1. Pattern Selection
The generator starts with a set of predefined black square patterns for the 5x5 grid. Each pattern is validated before use — it must keep all white cells connected, have at least 2 across and 2 down word slots, and not create any runs of fewer than 3 letters.

### 2. Dictionary Loading
Words are loaded from `.txt` files stored in the `/dictionaries` folder on LittleFS. Each file corresponds to a specific word length (3, 4, or 5 letters) for a given language (e.g. `english_3.txt`, `english_4.txt`, `english_5.txt`). Each entry contains a word and its clue.

### 3. Word Index
A `WordIndex` is built from the loaded dictionary. It maps each word length and character position to the list of word indices that have a specific letter at that position. This dramatically speeds up candidate lookup during backtracking.

### 4. Constraint Satisfaction + Backtracking
This is the core algorithm. At each step:
- It selects the most constrained unfilled word slot (fewest valid candidates — MRV heuristic)
- It retrieves candidate words that satisfy all letter constraints from intersecting already-placed words
- It places a word and recurses
- If no valid word exists for any slot, it backtracks and tries the next candidate

This continues until all slots are filled or all possibilities are exhausted, in which case a new pattern is tried.

### 5. Output
Once a valid puzzle is generated, it prints the grid and clues to Serial and triggers the LCD/OLED display for the physical hardware interface.

---

## Project Structure

```
├── main.cpp          # ESP32 entry point — runs setup() on boot
├── algorithm.h       # Data structures, function declarations
├── algorithm.cpp     # Core puzzle generation logic
├── dictionary.h      # DictEntry and WordDB type definitions
├── dictionary.cpp    # Dictionary loading and language detection
└── /dictionaries     # LittleFS folder containing word/clue .txt files
```

---

## How to Run

### Requirements
- ESP32 microcontroller
- [PlatformIO](https://platformio.org/) or Arduino IDE with ESP32 board support
- LittleFS filesystem with dictionary `.txt` files uploaded to the board

### Setup
1. Clone the repository
2. Upload the `/dictionaries` folder to the ESP32 filesystem using the LittleFS upload tool
3. Build and flash `main.cpp` to the ESP32
4. Open the Serial Monitor at **115200 baud**

### Expected Output
```
=== Crossword Puzzle Generator ===
[INFO] LittleFS mounted
Available languages: english
Loading dictionary: english
Generating puzzle...
Generation time: 312ms
=== 5x5 CROSSWORD ===
# # C A T
# A L O E
B R A I N
E A S E #
D E N # #

ACROSS:
  1. Feline pet -> CAT
  ...
DOWN:
  1. Tropical plant -> ALOE
  ...
```

---

## Technologies Used

| Technology | Purpose |
|---|---|
| C++ (ESP32/Arduino) | Core language |
| LittleFS | Onboard filesystem for dictionary storage |
| Constraint Satisfaction + Backtracking | Puzzle generation algorithm |
| Minimum Remaining Values (MRV) | Heuristic for slot selection |
| WordIndex (positional letter index) | Fast candidate filtering |
| ESP32 `esp_random()` | Seeded RNG for varied puzzle output |

---

## Algorithm Complexity

The backtracking algorithm is exponential in the worst case but is kept fast in practice by:
- The MRV heuristic (always filling the most constrained slot first)
- The positional word index (O(1) lookup per constraint instead of scanning all words)
- Randomized candidate ordering (produces varied puzzles across runs)
- Multiple pattern attempts with a configurable `max_attempts` limit (default: 20)

---

## Part of a Larger System

This algorithm is one component of a larger embedded crossword puzzle system built for ECE 1896 Senior Design at the University of Pittsburgh. The full system includes:
- OLED display for rendering the 5x5 letter grid
- LCD panel for displaying clues
- Rotary encoder for letter selection
- Push buttons for clue navigation and menu control
- Multi-language dictionary support via LittleFS
