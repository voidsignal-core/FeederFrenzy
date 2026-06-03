#include "../modeles/Parseur.h"
#include "../modeles/Constants.h"
#include <iostream>
#include <fstream>
#include <sstream>

bool Parseur::loadConfiguration(const std::string &filename,
                                std::vector<Mangeoire> &mangeoires,
                                std::vector<Animal> &animaux)
{
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << Message::FILE_NOT_OPEN << filename << Message::BACKSLASH << std::endl;
        return false; 
    }

    std::string line;
    int animal_id = 1;
    int mangeoire_id = 1;

    while (std::getline(file, line)) {
        if (line.empty()) continue; 

        std::stringstream bufferstring(line);
        char type;
        
        if (!(bufferstring >> type)) continue; 

        // 1. Traitement Mangeoire
        if (type == 'M') {
            int volume;
            if (bufferstring >> volume) {
                // Validation de la valeur obligatoire
                if (volume < 0) {
                    std::cerr << Message::FILE_INVALID_FORMAT << " (Negative volume)" << Message::BACKSLASH;
                    file.close();
                    return false;
                }

                int filling = 0;
                int counter = 0;
                
                bufferstring >> filling;
                bufferstring >> counter;

                // Validation des valeurs optionnelles
                if (filling < 0 || counter < 0) {
                    std::cerr << Message::FILE_INVALID_FORMAT << " (Negative refill values)" << Message::BACKSLASH;
                    file.close();
                    return false;
                }

                mangeoires.push_back(Mangeoire(mangeoire_id++, volume, filling, counter));
            } else {
                std::cerr << Message::FILE_INVALID_FORMAT << Message::BACKSLASH;
                file.close();
                return false;
            }
        }
        // 2. Traitement Animal
        else if (type == 'E' || type == 'C') {
            int portion, digest_time;
            if (bufferstring >> portion >> digest_time) {
                // Validation des valeurs de l'animal
                if (portion < 0 || digest_time < 0) {
                    std::cerr << Message::FILE_INVALID_FORMAT << " (Negative animal values)" << Message::BACKSLASH;
                    file.close();
                    return false;
                }

                animaux.push_back(Animal(animal_id++, type, portion, digest_time));
            } else {
                std::cerr << Message::FILE_INVALID_FORMAT << Message::BACKSLASH;
                file.close();
                return false;
            }
        }
    }

    file.close();
    return true;
}