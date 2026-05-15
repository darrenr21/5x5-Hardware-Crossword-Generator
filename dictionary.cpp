// dictionary.cpp
// ECE 1896 Senior Design - Team 13
// Author: Darren Ravichandra
//
// loads and validates word/clue pairs from .txt files in the dictionaries/ folder
// file naming format: {language}_{length}.txt  (e.g. english_3.txt, english_4.txt, english_5.txt)
// each line: WORD,clue text

#include "dictionary.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <filesystem>
#include <set>

namespace fs = std::filesystem;

static void to_upper(std::string& s) {
    for (char& c : s) c = static_cast<char>(std::toupper(c));
}

static std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    size_t end   = s.find_last_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

static bool is_alpha(const std::string& s) {
    for (char c : s)
        if (!std::isalpha(static_cast<unsigned char>(c))) return false;
    return !s.empty();
}

// reads one dictionary file and returns valid (word, clue) entries
// skips and logs any lines that are malformed, wrong length, or non-alphabetic
static std::vector<DictEntry> parse_file(const std::string& filepath, int expected_length) {
    std::vector<DictEntry> entries;
    std::vector<std::string> errors;

    std::ifstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error("Could not open file: " + filepath);

    std::string raw_line;
    int line_num = 0;

    while (std::getline(file, raw_line)) {
        line_num++;
        std::string line = trim(raw_line);

        if (line.empty() || line[0] == '#') continue;

        size_t comma_pos = line.find(',');
        if (comma_pos == std::string::npos) {
            errors.push_back("  Line " + std::to_string(line_num) + ": missing comma — '" + line + "'");
            continue;
        }

        // split on first comma only so clues can contain commas
        std::string word = trim(line.substr(0, comma_pos));
        std::string clue = trim(line.substr(comma_pos + 1));

        to_upper(word);

        if ((int)word.size() != expected_length) {
            errors.push_back("  Line " + std::to_string(line_num) + ": '" + word + "' has " +
                             std::to_string(word.size()) + " letters (expected " +
                             std::to_string(expected_length) + ")");
            continue;
        }

        if (!is_alpha(word)) {
            errors.push_back("  Line " + std::to_string(line_num) +
                             ": '" + word + "' contains non-letter characters");
            continue;
        }

        entries.push_back({word, clue});
    }

    if (!errors.empty()) {
        std::cout << "WARNING — " << errors.size() << " invalid entries in " << filepath << ":\n";
        for (const auto& e : errors) std::cout << e << "\n";
    }

    return entries;
}

WordDB load_dictionary(const std::string& language, const std::string& dict_dir) {
    WordDB word_db;

    for (int length : SUPPORTED_LENGTHS) {
        std::string filename = language + "_" + std::to_string(length) + ".txt";
        std::string filepath = dict_dir + "/" + filename;

        if (!fs::exists(filepath)) {
            throw std::runtime_error(
                "Dictionary file not found: " + filepath +
                "\nExpected at: " + fs::absolute(filepath).string()
            );
        }

        auto entries = parse_file(filepath, length);
        word_db[length] = entries;

        std::cout << "[DICT] Loaded " << entries.size() << " words from " << filename << "\n";
    }

    return word_db;
}

std::vector<std::string> list_available_languages(const std::string& dict_dir) {
    std::vector<std::string> complete;

    if (!fs::exists(dict_dir)) return complete;

    std::set<std::string> candidates;
    for (const auto& entry : fs::directory_iterator(dict_dir)) {
        std::string fname = entry.path().filename().string();

        if (fname.size() < 4 || fname.substr(fname.size() - 4) != ".txt") continue;

        std::string base = fname.substr(0, fname.size() - 4);
        size_t underscore = base.rfind('_');
        if (underscore == std::string::npos) continue;

        std::string lang       = base.substr(0, underscore);
        std::string length_str = base.substr(underscore + 1);

        bool is_num = !length_str.empty() &&
                      std::all_of(length_str.begin(), length_str.end(), ::isdigit);
        if (is_num) candidates.insert(lang);
    }

    // only include languages with all required length files
    for (const auto& lang : candidates) {
        bool has_all = true;
        for (int l : SUPPORTED_LENGTHS) {
            std::string path = dict_dir + "/" + lang + "_" + std::to_string(l) + ".txt";
            if (!fs::exists(path)) { has_all = false; break; }
        }
        if (has_all) complete.push_back(lang);
    }

    std::sort(complete.begin(), complete.end());
    return complete;
}

std::map<int, int> get_word_stats(const WordDB& word_db) {
    std::map<int, int> stats;
    for (const auto& [length, words] : word_db)
        stats[length] = static_cast<int>(words.size());
    return stats;
}
