#ifndef PARSEUR_H
#define PARSEUR_H

#include <string>
#include <vector>
#include "Mangeoire.h"
#include "Animal.h"

/**
 * @brief Utility class responsible for parsing the simulation configuration file.
 * @details Reads a text file line by line to extract definitions for both
 * Mangeoire (feeders) and Animal instances, populating the simulation vectors.
 */
class Parseur {
public:
    /**
     * @brief Loads the simulation configuration from a specified text file.
     * @details Parses lines starting with 'M' for feeders and 'E'/'C' for animals.
     * Generates unique IDs sequentially during the reading process.
     * @param filename The path to the configuration text file.
     * @param mangeoires Reference to the vector where parsed feeders will be stored.
     * @param animaux Reference to the vector where parsed animals will be stored.
     * @return true if the file was successfully opened and read, false otherwise.
     */
    static bool loadConfiguration(const std::string& filename, 
        std::vector<Mangeoire>& mangeoires, 
        std::vector<Animal>& animaux
    );
};

#endif