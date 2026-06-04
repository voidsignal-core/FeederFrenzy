#include "modeles/Mangeoire.h"

// Liens avec les variables globales de synchronisation de l'horloge
extern pthread_mutex_t clock_mutex;
extern pthread_cond_t clock_cond;
extern volatile bool active_simulation;
extern volatile int current_sim_tick;

Mangeoire::Mangeoire(int id, int volume, int filling, int counter)
    : id(id), volume(volume), filling(filling), counter(counter), filling_timer(counter), refill_count(0) {
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
int Mangeoire::getRefillCount() const { return refill_count; }

//---------------------
// MÉTHODES PRINCIPALES
//---------------------

bool Mangeoire::consume(int quantity) {
    pthread_mutex_lock(&volume_mutex);

    // RÈGLE STRICTE : Si le timer est à 0, la mangeoire est en plein refill. Accès refusé.
    if (counter > 0 && filling_timer == 0) {
        pthread_mutex_unlock(&volume_mutex);
        return false; 
    }

    // Logique de consommation standard
    bool success = false;
    if (volume >= quantity) {
        volume -= quantity;
        success = true;
    }

    pthread_mutex_unlock(&volume_mutex);
    return success;
}

void Mangeoire::refill() {
    if (filling == 0) return; // Aucun remplissage configuré

    pthread_mutex_lock(&volume_mutex);

    if (counter > 0) {
        filling_timer--;

        // Déclenchement de la recharge au passage à 0
        if (filling_timer == 0) {
            volume += filling;
            refill_count++;
        } 
        // Réamorçage du timer au tic suivant la recharge
        else if (filling_timer < 0) {
            filling_timer = counter;
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

        // Barrière de synchronisation pour éviter les réveils spurieux
        while (current_sim_tick == last_processed_tick && active_simulation) {
            pthread_cond_wait(&clock_cond, &clock_mutex);
        }

        if (!active_simulation) {
            pthread_mutex_unlock(&clock_mutex);
            break;
        }

        // On mémorise qu'on est en train de traiter ce tic
        last_processed_tick = current_sim_tick;
        pthread_mutex_unlock(&clock_mutex);

        // Traitement autonome de la recharge
        if (active_simulation) {
            self->refill();
        }
    }
    return nullptr;
}