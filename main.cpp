// main.cpp
// ECE 1896 Senior Design - Team 13
// Author: Darren Ravichandra
// setup() runs once on boot — loads dictionary, generates puzzle, prints it.
// loop() is empty — puzzle generation happens on demand not continuously.

#include <Arduino.h>
#include <LittleFS.h>
#include "algorithm.h"
#include "dictionary.h"

void setup() {
    Serial.begin(115200);
    delay(1000);  // give serial time to connect
    Serial.println("\n=== Crossword Puzzle Generator ===");

    if (!LittleFS.begin(true)) {
        Serial.println("[ERROR] LittleFS mount failed");
        return;
    }
    Serial.println("[INFO] LittleFS mounted");

    //Show available languages
    auto languages = list_available_languages("/dictionaries");
    Serial.print("Available languages: ");
    for (size_t i = 0; i < languages.size(); i++) {
        if (i > 0) Serial.print(", ");
        Serial.print(languages[i].c_str());
    }
    Serial.println();

    //Load dictionary
    std::string chosen_language = "english";
    Serial.print("\nLoading dictionary: ");
    Serial.println(chosen_language.c_str());

    WordDB word_db = load_dictionary(chosen_language, "/dictionaries");
    if (word_db.empty()) {
        Serial.println("[ERROR] Dictionary failed to load");
        return;
    }

    //Print memory after loading
    Serial.print("Free heap after load: ");
    Serial.println(ESP.getFreeHeap());
    Serial.print("Free PSRAM after load: ");
    Serial.println(ESP.getFreePsram());

    // ── Generate puzzle ───────────────────────────────────────────────────
    Serial.println("\nGenerating puzzle...");
    unsigned long start = millis();

    auto result = generate_puzzle(word_db, chosen_language);

    unsigned long elapsed = millis() - start;
    Serial.print("Generation time: ");
    Serial.print(elapsed);
    Serial.println("ms");

    if (!result) {
        Serial.println("[ERROR] Puzzle generation failed");
        return;
    }

    //Print puzzle
    const PuzzleGrid& puzzle = *result;
    print_puzzle(puzzle);

    //Clue display demo (for debugging purposes)
    Serial.println("---------------------------------------------");
    Serial.println("LCD CLUE DISPLAY DEMO");
    Serial.println("---------------------------------------------");

    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            char cell = puzzle.grid[row][col];
            if (cell != BLACK_SQ && cell != EMPTY_SQ) {
                for (const auto& dir : {"across", "down"}) {
                    const WordSlot* active = get_active_clue(puzzle, row, col, dir);
                    if (active) {
                        Serial.print("  Cell (");
                        Serial.print(row);
                        Serial.print(",");
                        Serial.print(col);
                        Serial.print(") ");
                        Serial.print(dir);
                        Serial.print(" -> \"");
                        Serial.print(active->clue.c_str());
                        Serial.println("\"");
                    }
                }
                goto done;
            }
        }
    }
    done:
    Serial.println("\n[INFO] Setup complete");
}

void loop() {
    // Empty for now — puzzle generation happens in setup()
    // In checkoff 3: this will handle button presses and user input
}
