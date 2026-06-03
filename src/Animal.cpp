#include "modeles/Animal.h"
#include <iostream>

// On lie les variables globales nécessaires à la synchronisation
extern pthread_mutex_t clock_mutex;
extern pthread_cond_t clock_cond;
extern volatile bool active_simulation;

Animal::Animal(int id, char type, int portion, int digest_time)
    : id(id), type(type), alive(true), portion(portion), digest_time(digest_time), digest_timer(0) {}

Animal::~Animal() {}

// GETTERS BASIQUES
int Animal::getId() const { return id; }
char Animal::getType() const { return type; }
int Animal::getPortion() const { return portion; }
int Animal::getDigestTime() const { return digest_time; }
bool Animal::isAlive() const { return alive; }

// MÉTHODES PRINCIPALES (VIDES POUR L'INSTANT)
void Animal::consume(std::vector<Mangeoire> &feeders) {
    // Vérification des conditions de base
    if (!alive) {
        std::cerr << "[ANIMAL " << id << "] Est déjà mort, ne peut pas manger." << std::endl;
        return;
    }

    if(digest_timer > 0) {
        digest_timer--;
        return;
    }

    if (feeders.empty()) {
        std::cerr << "[ANIMAL " << id << "] Aucune mangeoire disponible!" << std::endl;
        alive = false; // Meurt instantanément faute de nourriture
        return;
    }
    int random_index = rand() % feeders.size();
    
    bool eaten = feeders[random_index].consume(portion);
    if (eaten) {
        std::cout << "[ANIMAL " << id << "] A mangé " << portion
                    << " graines de la mangeoire " << feeders[random_index].getId() << "." << std::endl;
        digest_timer = digest_time; // Démarre la digestion
    } else {
        std::cout << "[ANIMAL " << id << "] N'a pas pu manger à cause d'un manque de graines à la mangeoire " << feeders[random_index].getId() << "." << std::endl;
        alive = false; // Meurt instantanément faute de nourriture
    }

}
void Animal::update() {}


// --- LA FONCTION DE THREAD (BANC D'ESSAI CORRIGÉ) ---
void* Animal::thread_func(void* arg) {
    // 1. CORRECTION : On cast d'abord vers la struct AnimalArgs reçue du main
    AnimalArgs* args = static_cast<AnimalArgs*>(arg);
    Animal* self = args->animal;
    std::vector<Mangeoire>& mangeoires = *(args->mangeoires);

    // Boucle tant que l'animal est vivant et que la simulation tourne
    while (self->isAlive() && active_simulation) {
        
        pthread_mutex_lock(&clock_mutex);
        
        // 2. L'animal s'endort sur la condition en relâchant le mutex
        pthread_cond_wait(&clock_cond, &clock_mutex);
        
        // On récupère le contrôle du mutex ici au réveil
        pthread_mutex_unlock(&clock_mutex); // CORRECTION : On déverrouille ICI pour laisser la place aux autres !

        // 3. On vérifie si on s'est réveillé pour de bon ou pour l'arrêt de la simu
        if (self->isAlive() && active_simulation) {
            std::cout << "[ANIMAL " << self->getId() << "] Réveillé sur un tic!" << std::endl;
            self->consume(mangeoires); // L'animal tente de manger
        }
    }

    return nullptr;
}