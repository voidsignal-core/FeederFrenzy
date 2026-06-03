#include <iostream>
#include <vector>
#include <unistd.h>
#include <pthread.h>
#include "modeles/Mangeoire.h"
#include "modeles/Animal.h"
#include "modeles/Parseur.h"

// Variables globales pour la synchronisation de l'horloge
pthread_mutex_t clock_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t clock_cond = PTHREAD_COND_INITIALIZER;
volatile bool active_simulation = true; // Flag pour contrôler la boucle de simulation

// Vecteur de mangeoires global pour permettre à l'horloge d'y accéder directement
std::vector<Mangeoire> mangeoires;

// --- LA FONCTION HORLOGE ---
void* horloge_func(void* arg) {
    int total_ticks = *static_cast<int*>(arg);
    
    for (int tick = 1; tick <= total_ticks; ++tick) {
        std::cout << "\n[HORLOGE] --- Début du TIC #" << tick << " ---" << std::endl;

        // Signal de réveil : on ouvre la barrière pour tous les animaux
        pthread_mutex_lock(&clock_mutex);
        pthread_cond_broadcast(&clock_cond); // "Tout le monde se réveille !"
        pthread_mutex_unlock(&clock_mutex);

        // Durée du tic : on laisse 100ms aux animaux pour faire leur action
        usleep(100000);
    }

    // Fin du temps de simulation : on ferme la marche
    std::cout << "\n[HORLOGE] Fin du temps imparti. Arrêt de la simulation." << std::endl;
    
    pthread_mutex_lock(&clock_mutex);
    active_simulation = false;          // On passe le flag à false pour briser le while des animaux
    pthread_cond_broadcast(&clock_cond); // Un dernier broadcast pour libérer ceux qui dorment
    pthread_mutex_unlock(&clock_mutex);
    
    return nullptr;
}

int main(int argc, char* argv[]) {
    // Variable pour stocker les animaux (mangeoires est globale en haut)
    std::vector<Animal> animaux;

    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <fichier_configuration> <temps_simulation>" << std::endl;
        return 1;
    }

    std::string nomFichier = argv[1];
    int tempsSimulation = std::stoi(argv[2]);
    std::cout << "Tentative de chargement de : " << nomFichier << std::endl;

    // Appel de ton parseur
    if (Parseur::loadConfiguration(nomFichier, mangeoires, animaux)) {
        std::cout << "\n--- SUCCÈS DU CHARGEMENT ---" << std::endl;
        std::cout << "Nombre de mangeoires chargées : " << mangeoires.size() << std::endl;
        std::cout << "Nombre d'animaux chargés     : " << animaux.size() << std::endl;

        std::cout << "\n--- INITIALISATION DU BANC D'ESSAI MULTITHREAD ---" << std::endl;

        // Tableaux pour stocker les identifiants de threads et les arguments alloués
        std::vector<pthread_t> animal_tids(animaux.size());
        std::vector<AnimalArgs*> arguments_alloues;

        // 1. Lancement des threads animaux
        for (size_t i = 0; i < animaux.size(); ++i) {
            // Allocation de l'argument attendu par Animal::thread_func
            AnimalArgs* args = new AnimalArgs{&animaux[i], &mangeoires};
            arguments_alloues.push_back(args);
            
            pthread_create(&animal_tids[i], NULL, Animal::thread_func, args);
        }

        // Laisser une fraction de seconde pour s'assurer que tous les threads animaux 
        // sont créés et bloqués sur le pthread_cond_wait avant le premier tick.
        usleep(10000);

        // 2. Lancement du thread de l'horloge
        pthread_t horloge_tid;
        pthread_create(&horloge_tid, NULL, horloge_func, &tempsSimulation);

        // 3. Attente de la fin de l'horloge
        pthread_join(horloge_tid, NULL);

        // 4. Attente de la fin de tous les threads animaux
        for (size_t i = 0; i < animaux.size(); ++i) {
            pthread_join(animal_tids[i], NULL);
        }

        // 5. Nettoyage de la mémoire dynamique des arguments
        for (auto args : arguments_alloues) {
            delete args;
        }

        std::cout << "\n[MAIN] Test terminé proprement. Tous les threads sont fermés." << std::endl;

    } else {
        std::cout << "\n--- ÉCHEC DU CHARGEMENT ---" << std::endl;
        std::cout << "Vérifie que le fichier existe et qu'il n'a pas de valeurs négatives." << std::endl;
        return 1;
    }

    return 0;
}