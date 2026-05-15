// main.cpp
// ECE 1896 Senior Design - Team 13
// Author: Darren Ravichandra
// run: ./crossword

#include "algorithm.h"
#include "dictionary.h"

#include <iostream>
#include <chrono>

int main() {
    auto languages = list_available_languages("dictionaries");
    std::cout << "Available languages: ";
    for (size_t i = 0; i < languages.size(); i++) {
        if (i > 0) std::cout << ", ";
        std::cout << languages[i];
    }
    std::cout << "\n";

    // change this to switch languages
    std::string chosen_language = "english";
    std::cout << "\nGenerating puzzle in: " << chosen_language << "\n";

    WordDB word_db;
    try {
        word_db = load_dictionary(chosen_language, "dictionaries");
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << "\n";
        return 1;
    }
    std::cout << "\n";

    auto start  = std::chrono::high_resolution_clock::now();
    auto result = generate_puzzle(word_db, chosen_language);
    auto end    = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();

    std::cout << "Generation time: " << elapsed << "s\n\n";

    if (!result) {
        std::cout << "Puzzle generation failed.\n";
        std::cout << "Try expanding the dictionary files or increasing max_attempts.\n";
        return 1;
    }

    print_puzzle(*result);
    return 0;
}
