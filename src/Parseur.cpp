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
        return false; // Crucial pour arrêter la fonction ici !
    }

    std::string line;
    int animal_id = 1;
    int mangeoire_id = 1;

    while (std::getline(file, line)) {
        if (line.empty()) continue; // On saute les lignes vides automatiquement

        std::stringstream bufferstring(line);
        char type;
        
        // 1. On extrait le premier caractère (le type de ligne)
        if (!(bufferstring >> type)) continue; 

        // 2. Traitement si c'est une Mangeoire
        if (type == 'M') {
            int volume;
            // Le volume initial est obligatoire
            if (bufferstring >> volume) {
                int filling = 0;
                int counter = 0;
                
                // On tente d'extraire les valeurs optionnelles.
                // Si la ligne s'arrête avant, elles restent à 0.
                bufferstring >> filling;
                bufferstring >> counter;

                // Ajout au vecteur (le constructeur gère le reste)
                mangeoires.push_back(Mangeoire(mangeoire_id++, volume, filling, counter));
            } else {
                std::cerr << Message::FILE_INVALID_FORMAT << Message::BACKSLASH;
            }
        }
        // 3. Traitement si c'est un Animal (Écureuil ou Cardinal)
        else if (type == 'E' || type == 'C') {
            int portion, digest_time;
            // Pour les animaux, tous les paramètres sont obligatoires
            if (bufferstring >> portion >> digest_time) {
                animaux.push_back(Animal(animal_id++, type, portion, digest_time));
            } else {
                std::cerr << Message::FILE_INVALID_FORMAT << Message::BACKSLASH;
            }
        }
    }

    file.close();
    return true;
}