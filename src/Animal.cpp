#include "modeles/Animal.h"
#include <random>

// Liens avec les variables globales de synchronisation de l'horloge
extern pthread_mutex_t clock_mutex;
extern pthread_cond_t clock_cond;
extern volatile bool active_simulation;
extern volatile int current_sim_tick;

Animal::Animal(int id, char type, int portion, int digest_time)
    : id(id), type(type), alive(true), portion(portion), digest_time(digest_time), digest_timer(0)
    , total_eaten(0), meals_count(0) {}

Animal::~Animal() {}

//-----------------
// GETTERS BASIQUES
//-----------------

int Animal::getId() const { return id; }
char Animal::getType() const { return type; }
int Animal::getPortion() const { return portion; }
int Animal::getDigestTime() const { return digest_time; }
bool Animal::isAlive() const { return alive; }
int Animal::getTotalEaten() const { return total_eaten; }
int Animal::getMealsCount() const { return meals_count; }

//---------------------
// MÉTHODES PRINCIPALES
//---------------------

void Animal::consume(std::vector<Mangeoire> &feeders) {
    // Vérification des conditions de base
    if (!alive) return; // Si déjà mort, on ne fait rien

    // Gestion du cycle de digestion
    if(digest_timer > 0) {
        digest_timer--;
        return;
    }

    if (feeders.empty()) {
        alive = false; // Meurt instantanément faute de nourriture
        return;
    }

    // Sélection de la mangeoire
    static thread_local std::mt19937 generator(std::random_device{}());
    std::uniform_int_distribution<int> distribution(0, feeders.size() - 1);
    
    int random_index = distribution(generator);
    
    bool eaten = feeders[random_index].consume(portion);
    
    if (eaten) {
        digest_timer = digest_time; // Début de la digestion
        total_eaten++;
    } else {
        alive = false; 
    }
    meals_count++;
}

void* Animal::thread_func(void* arg) {
    AnimalArgs* args = static_cast<AnimalArgs*>(arg);
    Animal* self = args->animal;
    std::vector<Mangeoire>& mangeoires = *(args->mangeoires);
    int last_processed_tick = 0;

    while (self->isAlive() && active_simulation) {
        pthread_mutex_lock(&clock_mutex);
        
        // Attente du top de l'horloge
        while (current_sim_tick == last_processed_tick && active_simulation) {
            pthread_cond_wait(&clock_cond, &clock_mutex);
        }
        
        // Si on sort de la boucle parce que la simulation est finie, on s'en va
        if (!active_simulation || !self->alive) {
            pthread_mutex_unlock(&clock_mutex);
            break;
        }

        // On mémorise qu'on est en train de traiter ce tic
        last_processed_tick = current_sim_tick;
        pthread_mutex_unlock(&clock_mutex); 

        // Exécution de la routine métabolique
        if (self->isAlive() && active_simulation) {
            self->consume(mangeoires); 
        }
    }

    return nullptr;
}