// algorithm.cpp
// ECE 1896 Senior Design - Team 13
// Author: Darren Ravichandra

#include "algorithm.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <chrono>
#include <cassert>
#include <cstring>

static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());

// per-attempt step and time budgets
// successful generations usually finish in under 10,000 steps
// anything beyond 50,000 is almost certainly a dead end, so we cut it off and try a new pattern
static const int MAX_BACKTRACK_STEPS = 50000;
static const uint32_t MAX_ATTEMPT_MS = 5000;

static uint32_t millis_now() {
    static const auto t0 = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count();
}

// returns the list of (row, col) cells that this word slot occupies
std::vector<Cell> WordSlot::cells() const {
    std::vector<Cell> result;
    for (int i = 0; i < length; i++) {
        if (direction == "across")
            result.push_back({start_row, start_col + i});
        else
            result.push_back({start_row + i, start_col});
    }
    return result;
}

// builds the grid from a black square pattern and finds all valid word slots
PuzzleGrid::PuzzleGrid(const Pattern& pattern, const std::string& lang)
    : language(lang)
{
    for (int r = 0; r < GRID_SIZE; r++)
        for (int c = 0; c < GRID_SIZE; c++)
            grid[r][c] = EMPTY_SQ;

    for (const auto& [r, c] : pattern)
        grid[r][c] = BLACK_SQ;

    // scan each row for across slots
    for (int r = 0; r < GRID_SIZE; r++) {
        int c = 0;
        while (c < GRID_SIZE) {
            if (grid[r][c] != BLACK_SQ) {
                int start_c = c;
                while (c < GRID_SIZE && grid[r][c] != BLACK_SQ) c++;
                int length = c - start_c;
                if (length >= 3)
                    slots.push_back({"across", r, start_c, length, "", ""});
            } else {
                c++;
            }
        }
    }

    // scan each column for down slots
    for (int col = 0; col < GRID_SIZE; col++) {
        int r = 0;
        while (r < GRID_SIZE) {
            if (grid[r][col] != BLACK_SQ) {
                int start_r = r;
                while (r < GRID_SIZE && grid[r][col] != BLACK_SQ) r++;
                int length = r - start_r;
                if (length >= 3)
                    slots.push_back({"down", start_r, col, length, "", ""});
            } else {
                r++;
            }
        }
    }
}

const std::vector<Pattern> PATTERNS = {
    { {0,0},{0,1},{1,0},{3,4},{4,3},{4,4} },
    { {0,3},{0,4},{1,4},{3,0},{4,0},{4,1} },
    { {0,0},{4,4} },
    { {0,4},{4,0} },
    { {0,0},{0,1},{4,3},{4,4} },
    { {0,3},{0,4},{4,0},{4,1} },
};

bool is_valid_pattern(const Pattern& pattern) {
    char grid[GRID_SIZE][GRID_SIZE];
    for (int r = 0; r < GRID_SIZE; r++)
        for (int c = 0; c < GRID_SIZE; c++)
            grid[r][c] = (pattern.count({r, c}) ? BLACK_SQ : EMPTY_SQ);

    std::vector<Cell> white_cells;
    for (int r = 0; r < GRID_SIZE; r++)
        for (int c = 0; c < GRID_SIZE; c++)
            if (grid[r][c] != BLACK_SQ)
                white_cells.push_back({r, c});

    if (white_cells.empty()) return false;

    // flood fill to check all white cells are connected
    std::set<Cell> visited;
    std::vector<Cell> stack = {white_cells[0]};
    const int dr[] = {-1, 1, 0, 0};
    const int dc[] = {0, 0, -1, 1};

    while (!stack.empty()) {
        auto [r, c] = stack.back(); stack.pop_back();
        if (visited.count({r, c})) continue;
        visited.insert({r, c});
        for (int d = 0; d < 4; d++) {
            int nr = r + dr[d], nc = c + dc[d];
            if (nr >= 0 && nr < GRID_SIZE && nc >= 0 && nc < GRID_SIZE
                && grid[nr][nc] != BLACK_SQ && !visited.count({nr, nc}))
                stack.push_back({nr, nc});
        }
    }

    if (visited.size() != white_cells.size()) return false;

    // need at least 2 across and 2 down slots
    PuzzleGrid temp(pattern);
    int across_count = 0, down_count = 0;
    for (const auto& s : temp.slots) {
        if (s.direction == "across") across_count++;
        else down_count++;
    }
    if (across_count < 2 || down_count < 2) return false;

    // every white cell must belong to at least one slot
    std::set<Cell> covered;
    for (const auto& slot : temp.slots)
        for (const auto& cell : slot.cells())
            covered.insert(cell);
    if (covered != std::set<Cell>(white_cells.begin(), white_cells.end()))
        return false;

    // no row or column can have a white run shorter than 3
    for (int r = 0; r < GRID_SIZE; r++) {
        int run = 0;
        for (int c = 0; c < GRID_SIZE; c++) {
            if (grid[r][c] != BLACK_SQ) run++;
            else { if (run > 0 && run < 3) return false; run = 0; }
        }
        if (run > 0 && run < 3) return false;
    }
    for (int c = 0; c < GRID_SIZE; c++) {
        int run = 0;
        for (int r = 0; r < GRID_SIZE; r++) {
            if (grid[r][c] != BLACK_SQ) run++;
            else { if (run > 0 && run < 3) return false; run = 0; }
        }
        if (run > 0 && run < 3) return false;
    }

    return true;
}

// builds a position-indexed lookup table so get_candidates() can filter words quickly
// instead of scanning the entire dictionary, it looks up only words that match a specific letter at a specific position
WordIndex build_index(const WordDB& word_db) {
    WordIndex index;
    for (const auto& [length, entries] : word_db) {
        index[length].resize(length);
        for (const auto& entry : entries) {
            for (int pos = 0; pos < length; pos++) {
                char letter = entry.word[pos];
                index[length][pos][letter].push_back(entry);
            }
        }
    }
    return index;
}

// returns all dictionary words that can legally fit into the given slot
// based on letters already placed in the grid from crossing words
std::vector<DictEntry> get_candidates(
    const WordSlot& slot,
    const char grid[GRID_SIZE][GRID_SIZE],
    const WordDB& word_db,
    const std::set<std::string>& used_words,
    const WordIndex* index)
{
    int length = slot.length;
    auto slot_cells = slot.cells();

    // collect constraints: positions where crossing words have already placed a letter
    std::map<int, char> constraints;
    for (int i = 0; i < (int)slot_cells.size(); i++) {
        auto [r, c] = slot_cells[i];
        char existing = grid[r][c];
        if (existing != EMPTY_SQ && existing != BLACK_SQ)
            constraints[i] = existing;
    }

    std::vector<DictEntry> candidates;
    if (index && index->count(length)) {
        const auto& len_index = index->at(length);

        if (!constraints.empty()) {
            // start from the most constrained position (fewest matching words)
            // then filter down from there — faster than starting with a big list
            int best_pos = -1;
            size_t best_count = SIZE_MAX;
            for (const auto& [pos, letter] : constraints) {
                auto it = len_index[pos].find(letter);
                size_t count = (it != len_index[pos].end()) ? it->second.size() : 0;
                if (count < best_count) {
                    best_count = count;
                    best_pos = pos;
                }
            }

            if (best_pos >= 0) {
                char best_letter = constraints.at(best_pos);
                auto it = len_index[best_pos].find(best_letter);
                if (it != len_index[best_pos].end())
                    candidates = it->second;

                for (const auto& [pos, letter] : constraints) {
                    if (pos == best_pos) continue;
                    candidates.erase(
                        std::remove_if(candidates.begin(), candidates.end(),
                            [&](const DictEntry& e) { return e.word[pos] != letter; }),
                        candidates.end()
                    );
                }
            }
        } else {
            auto it = word_db.find(length);
            if (it != word_db.end()) candidates = it->second;
        }
    } else {
        // fallback: linear scan if no index available
        auto it = word_db.find(length);
        if (it != word_db.end()) {
            for (const auto& entry : it->second) {
                bool fits = true;
                for (const auto& [pos, letter] : constraints) {
                    if (entry.word[pos] != letter) { fits = false; break; }
                }
                if (fits) candidates.push_back(entry);
            }
        }
    }

    // remove words already placed in the grid
    candidates.erase(
        std::remove_if(candidates.begin(), candidates.end(),
            [&](const DictEntry& e) { return used_words.count(e.word) > 0; }),
        candidates.end()
    );

    // shuffle so we get a different puzzle each run
    std::shuffle(candidates.begin(), candidates.end(), rng);
    return candidates;
}

bool backtrack(
    std::vector<WordSlot>& slots,
    char grid[GRID_SIZE][GRID_SIZE],
    const WordDB& word_db,
    std::set<std::string>& used_words,
    const WordIndex* word_index,
    int& step_counter,
    int step_limit,
    uint32_t deadline_ms)
{
    step_counter++;
    if (step_counter > step_limit) return false;
    if (millis_now() > deadline_ms) return false;

    std::vector<WordSlot*> remaining;
    for (auto& slot : slots)
        if (slot.answer.empty()) remaining.push_back(&slot);

    if (remaining.empty()) return true;

    // MRV (minimum remaining values): always fill the slot with the fewest valid candidates first
    // this reduces the search space significantly compared to filling slots in arbitrary order
    WordSlot* best_slot = nullptr;
    size_t best_count = SIZE_MAX;

    for (auto* slot : remaining) {
        auto cands = get_candidates(*slot, grid, word_db, used_words, word_index);
        size_t n = cands.size();
        if (n == 0) return false;
        bool better = (n < best_count) ||
                      (n == best_count && best_slot && slot->length > best_slot->length);
        if (better) {
            best_count = n;
            best_slot = slot;
        }
    }

    if (!best_slot) return false;

    auto candidates = get_candidates(*best_slot, grid, word_db, used_words, word_index);
    if (candidates.empty()) return false;

    auto slot_cells = best_slot->cells();

    // save current grid cells so we can undo this placement if needed
    std::vector<char> saved;
    for (const auto& [r, c] : slot_cells)
        saved.push_back(grid[r][c]);

    for (const auto& entry : candidates) {
        // place the word
        for (int i = 0; i < (int)slot_cells.size(); i++) {
            auto [r, c] = slot_cells[i];
            grid[r][c] = entry.word[i];
        }
        best_slot->answer = entry.word;
        best_slot->clue   = entry.clue;
        used_words.insert(entry.word);

        if (backtrack(slots, grid, word_db, used_words, word_index,
                      step_counter, step_limit, deadline_ms))
            return true;

        // undo the placement and try the next candidate
        for (int i = 0; i < (int)slot_cells.size(); i++) {
            auto [r, c] = slot_cells[i];
            grid[r][c] = saved[i];
        }
        best_slot->answer.clear();
        best_slot->clue.clear();
        used_words.erase(entry.word);

        if (step_counter > step_limit || millis_now() > deadline_ms) return false;
    }

    return false;
}

std::optional<PuzzleGrid> generate_puzzle(
    const WordDB& word_db,
    const std::string& language,
    int max_attempts)
{
    std::vector<Pattern> valid_patterns;
    for (const auto& p : PATTERNS)
        if (is_valid_pattern(p)) valid_patterns.push_back(p);

    if (valid_patterns.empty()) {
        std::cout << "[ERROR] No valid patterns available.\n";
        return std::nullopt;
    }

    WordIndex word_index = build_index(word_db);

    std::uniform_int_distribution<int> dist(0, (int)valid_patterns.size() - 1);

    for (int attempt = 0; attempt < max_attempts; attempt++) {
        const Pattern& pattern = valid_patterns[dist(rng)];
        PuzzleGrid puzzle(pattern, language);
        std::set<std::string> used_words;

        int step_counter = 0;
        uint32_t deadline_ms = millis_now() + MAX_ATTEMPT_MS;

        bool ok = backtrack(puzzle.slots, puzzle.grid, word_db, used_words,
                            &word_index, step_counter, MAX_BACKTRACK_STEPS, deadline_ms);

        if (ok) {
            std::cout << "[INFO] Puzzle generated (attempt "
                      << (attempt + 1) << "/" << max_attempts
                      << ", steps: " << step_counter << ")\n";
            return puzzle;
        }

        bool budget_hit = (step_counter > MAX_BACKTRACK_STEPS) || (millis_now() > deadline_ms);
        std::cout << "[ATTEMPT] Attempt " << (attempt + 1)
                  << (budget_hit ? " timed out" : " failed")
                  << " (steps: " << step_counter << "), retrying...\n";
    }

    std::cout << "[ERROR] Generation failed after " << max_attempts << " attempts.\n";
    return std::nullopt;
}

const WordSlot* get_active_clue(
    const PuzzleGrid& puzzle,
    int selected_row,
    int selected_col,
    const std::string& direction)
{
    for (const auto& slot : puzzle.slots) {
        if (slot.direction != direction) continue;
        for (const auto& [r, c] : slot.cells()) {
            if (r == selected_row && c == selected_col)
                return &slot;
        }
    }
    return nullptr;
}

void print_grid(const PuzzleGrid& puzzle) {
    std::cout << "\n  +";
    for (int i = 0; i < GRID_SIZE; i++) std::cout << "---+";
    std::cout << "\n";

    for (int r = 0; r < GRID_SIZE; r++) {
        std::cout << "  |";
        for (int c = 0; c < GRID_SIZE; c++) {
            char cell = puzzle.grid[r][c];
            if (cell == BLACK_SQ)       std::cout << "###|";
            else if (cell == EMPTY_SQ)  std::cout << "   |";
            else                        std::cout << " " << cell << " |";
        }
        std::cout << "\n  +";
        for (int i = 0; i < GRID_SIZE; i++) std::cout << "---+";
        std::cout << "\n";
    }
    std::cout << "\n";
}

void print_clues(const PuzzleGrid& puzzle) {
    std::cout << "ACROSS:\n";
    int i = 1;
    for (const auto& slot : puzzle.slots) {
        if (slot.direction != "across") continue;
        std::cout << "  " << i++ << ". (R" << slot.start_row << "C" << slot.start_col
                  << ", " << slot.length << " letters) " << slot.clue << "\n";
        std::cout << "     Answer: " << slot.answer << "\n";
    }

    std::cout << "\nDOWN:\n";
    i = 1;
    for (const auto& slot : puzzle.slots) {
        if (slot.direction != "down") continue;
        std::cout << "  " << i++ << ". (R" << slot.start_row << "C" << slot.start_col
                  << ", " << slot.length << " letters) " << slot.clue << "\n";
        std::cout << "     Answer: " << slot.answer << "\n";
    }
    std::cout << "\n";
}

void print_puzzle(const PuzzleGrid& puzzle) {
    std::cout << "5x5 CROSSWORD PUZZLE\n";
    std::cout << "Language: " << puzzle.language << "\n";
    print_grid(puzzle);
    print_clues(puzzle);
}
