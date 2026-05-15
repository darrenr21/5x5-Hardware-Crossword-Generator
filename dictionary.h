#pragma once
// dictionary.h
// ECE 1896 Senior Design - Team 13
// Author: Darren Ravichandra
//
// validation for AI-generated dictionary words/clues
// (writing thousands of clues by hand would be way too tedious)

#include <string>
#include <vector>
#include <map>
#include <stdexcept>

struct DictEntry {
    std::string word;
    std::string clue;
};

// maps word length to a list of (word, clue) pairs
using WordDB = std::map<int, std::vector<DictEntry>>;

const std::vector<int> SUPPORTED_LENGTHS = {3, 4, 5};

WordDB load_dictionary(const std::string& language, const std::string& dict_dir = "dictionaries");

// returns languages that have complete sets of all 3 required files (3, 4, 5 letters)
std::vector<std::string> list_available_languages(const std::string& dict_dir = "dictionaries");

// returns word count per length — useful for verifying files loaded correctly
std::map<int, int> get_word_stats(const WordDB& word_db);
