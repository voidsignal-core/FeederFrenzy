#include "modeles/Mangeoire.h"


#include <iostream>

// On lie les variables globales nécessaires à la synchronisation
extern pthread_mutex_t clock_mutex;
extern pthread_cond_t clock_cond;
extern volatile bool active_simulation;
extern volatile int current_sim_tick; // Compteur de tic global

Mangeoire::Mangeoire(int id, int volume, int filling, int counter)
    : id(id), volume(volume), filling(filling), counter(counter), filling_timer(counter) {
    pthread_mutex_init(&volume_mutex, NULL);
}

Mangeoire::~Mangeoire() {
    pthread_mutex_destroy(&volume_mutex);
}

//-----------------
// GETTERS BASIQUES
//-----------------

int Mangeoire::getId() const { return id; }
int Mangeoire::getVolume() const { return volume; }
int Mangeoire::getFilling() const { return filling; }
int Mangeoire::getCounter() const { return counter; }

//---------------------
// MÉTHODES PRINCIPALES
//---------------------

bool Mangeoire::consume(int quantity) {
    pthread_mutex_lock(&volume_mutex);

    // RÈGLE TRAGIQUE : Si le filling_timer est à 0, la mangeoire est en plein refill.
    // L'accès est coupé, la méthode renvoie false et l'animal va mourir.
    if (counter > 0 && filling_timer == 0) {
        std::cout << "  [MANGEOIRE " << id << "] ACCÈS REFUSÉ (Remplissage en cours) !" << std::endl;
        pthread_mutex_unlock(&volume_mutex);
        return false; 
    }

    // Logique de consommation standard
    bool success = false;
    if (volume >= quantity) {
        volume -= quantity;
        success = true;
    } else {
        std::cout << "  [MANGEOIRE " << id << "] Vide ou insuffisante pour la portion demandée." << std::endl;
    }

    pthread_mutex_unlock(&volume_mutex);
    return success;
}
void Mangeoire::update() {}

void Mangeoire::refill() {
    // 1. Si filling == 0 → On ne fait jamais de refill
    if (filling == 0) return; 

    pthread_mutex_lock(&volume_mutex);

    // 2. Si counter > 0 → On décrémente et on gère le cycle
    if (counter > 0) {
        filling_timer--;

        // C'est le moment du refill : on augmente le volume et on bloque l'accès
        if (filling_timer == 0) {
            volume += filling;
            std::cout << "[MANGEOIRE " << id << "] ⚙️ RECHARGE ! +" << filling 
                      << " graines. Accès VERROUILLÉ pour ce tic. Volume actuel : " << volume << std::endl;
        } 
        // Le tic de blocage est passé, on reset le timer pour le prochain cycle
        else if (filling_timer < 0) {
            filling_timer = counter; // Le tic actuel compte déjà pour -1 dans le nouveau cycle
        }
    }

    pthread_mutex_unlock(&volume_mutex);
}

void* Mangeoire::thread_func(void* arg) {
    MangeoireArgs* args = static_cast<MangeoireArgs*>(arg);
    Mangeoire* self = args->mangeoire;

    int last_processed_tick = 0;

    while (active_simulation) {
        pthread_mutex_lock(&clock_mutex);

        while (current_sim_tick == last_processed_tick && active_simulation) {
            pthread_cond_wait(&clock_cond, &clock_mutex);
        }

        if (!active_simulation) {
            pthread_mutex_unlock(&clock_mutex);
            break;
        }

        last_processed_tick = current_sim_tick;

        pthread_mutex_unlock(&clock_mutex);


        // On exécute la logique de recharge autonome
        if (active_simulation) {
            self->refill();
        }
    }
    return nullptr;
}