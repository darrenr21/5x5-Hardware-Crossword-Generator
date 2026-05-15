#pragma once
// algorithm.h
// ECE 1896 Senior Design - Team 13
// Author: Darren Ravichandra

#include "dictionary.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <optional>
#include <cstdint>

// grid constants
const int  GRID_SIZE = 5;
const char BLACK_SQ  = '#';
const char EMPTY_SQ  = '\0';

using Cell    = std::pair<int, int>;
using Pattern = std::set<Cell>;

struct WordSlot {
    std::string direction;
    int start_row;
    int start_col;
    int length;
    std::string answer;
    std::string clue;

    std::vector<Cell> cells() const;
};

struct PuzzleGrid {
    std::string language;
    char grid[GRID_SIZE][GRID_SIZE];
    std::vector<WordSlot> slots;

    PuzzleGrid(const Pattern& pattern, const std::string& language = "english");

    char get_cell(int row, int col) const { return grid[row][col]; }
    void set_cell(int row, int col, char value) { grid[row][col] = value; }
};

extern const std::vector<Pattern> PATTERNS;

// checks connectivity, slot count, and no 2-letter runs
bool is_valid_pattern(const Pattern& pattern);

// WordIndex maps word length -> position -> letter -> list of matching entries
// used to quickly find candidate words during backtracking without scanning the whole dictionary
using WordIndex = std::map<int, std::vector<std::map<char, std::vector<DictEntry>>>>;

WordIndex build_index(const WordDB& word_db);

std::vector<DictEntry> get_candidates(
    const WordSlot& slot,
    const char grid[GRID_SIZE][GRID_SIZE],
    const WordDB& word_db,
    const std::set<std::string>& used_words,
    const WordIndex* index = nullptr
);

// backtracking solver — step_counter and deadline_ms are used to bail out early
// if the current pattern is taking too long, so we can try a different one
bool backtrack(
    std::vector<WordSlot>& slots,
    char grid[GRID_SIZE][GRID_SIZE],
    const WordDB& word_db,
    std::set<std::string>& used_words,
    const WordIndex* word_index,
    int& step_counter,
    int step_limit,
    uint32_t deadline_ms
);

std::optional<PuzzleGrid> generate_puzzle(
    const WordDB& word_db,
    const std::string& language = "english",
    int max_attempts = 20
);

const WordSlot* get_active_clue(
    const PuzzleGrid& puzzle,
    int selected_row,
    int selected_col,
    const std::string& direction = "across"
);

void print_grid(const PuzzleGrid& puzzle);
void print_clues(const PuzzleGrid& puzzle);
void print_puzzle(const PuzzleGrid& puzzle);
