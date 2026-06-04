# TP1 – Simulation Pthreads : Écosystème Producteurs-Consommateurs

## Description

Ce projet implémente une simulation multithreadée non-déterministe d'un écosystème en boucle fermée. Le système modélise les interactions dynamiques entre des **consommateurs autonomes (Animaux)** et des **producteurs passifs/actifs (Mangeoires)**, rythmés par un chef d'orchestre temporel commun : **l'Horloge**.

Chaque entités évolue indépendamment dans son propre thread et interagit avec les autres à travers des mécanismes de synchronisation.

## Compilation et exécution
### Compilation

Pour compiler le projet, utilisez les commandes suivantes dans le terminal à la racine du projet :

```bash
make debug   # -O0 pour débogage
make release # -O2 pour remise
make clean   # Nettoyage des fichiers objets et exécutables
```

### Exécution

Pour exécuter la simulation, utilisez la commande suivante en fournissant un fichier de configuration et le nombre de tics à simuler :
```bash
./simulation <fichier_config> <nombre_tics>
```

Si aucun n'argument est fourni, la simulation va donner un message d'erreur et se terminer.

## Architecture et choix de conception

Face à un cahier des charges flexible sur les choix de design, j'ai conçu une architecture modulaire et orientée objet. Structurée autour de la formule de synchronisation imposée, elle sépare nettement les composantes clés du système : les animaux, les mangeoires, la simulation et le module de synchronisation.

### Threads
La simulation est organisée en différents types de threads :
- **Thread principal** : Initialise la simulation, charge la configuration, lance les threads d'animaux et de mangeoires, puis se met en veille complète pendant la simulation. C'est lui qui récupère les résultats engendrés par les autres threads à la fin de la simulation et génère le rapport de statistiques final.
- **Thread d'horloge** : Véritable chef d'orchestre de la simulation. Il émet des tics à intervalles réguliers, réveille les entités à intervalles réguliers et envoie le signal de fermeture général.
- **Threads d'animaux ($N$ consommateurs)** : Chaque animal s'exécute dans son propre thread, où il attend les tics de l'horloge pour tenter de consommer des graines. Ils consomment la nourriture des mangeoires à chaque tic où ils ne digèrent pas. Ils meurent instantanément si l'accès à la nourriture échoue.
- **Threads de mangeoires ($M$ producteurs)** : Chaque mangeoire s'exécute dans son propre thread, où elle attend les tics de l'horloge pour produire des graines.

### Synchronisation
Dans l'architecture, la sécurité de l'entièreté de l'écosystème repose sur les primitives de synchronisation suivantes :

#### 1. Les Verrous d'Exclusion Mutuelle (`pthread_mutex_t`)

Les mutex sont utilisés pour garantir l'exclusion mutuelle lors de l'accès aux ressources partagées critiques du système :

* **`clock_mutex` (Horloge globale)** : Sécurise l'accès et l'incrémentation du compteur de tics global de la simulation, ainsi que la vérification de la condition de fin. Les threads d'animaux et de mangeoires acquièrent ce verrou pour consulter l'état de l'horloge en toute sécurité.
* **`volume_mutex` (Spécifique à chaque Mangeoire)** : Protège l'accès concurrent aux données internes de chaque instance de mangeoire (volume de graines et compte à rebours de recharge). Un animal doit acquérir le mutex de la mangeoire visée pour consommer, tout comme le thread de recharge doit le faire pour produire, évitant ainsi tout conflit.

#### 2. Les Variables de Condition (`pthread_cond_t`)

Les variables de condition permettent de coordonner le cycle de vie et le rythme de la simulation :

* **Barrière de synchronisation globale (`clock_cond`)** : Utilisée pour bloquer l'ensemble des threads (animaux et mangeoires) au début de chaque cycle. À chaque tic, le thread de l'horloge utilise `pthread_cond_broadcast` pour réveiller simultanément tous les acteurs afin qu'ils exécutent leurs actions respectives.
* **Gestion des réveils spurieux (*spurious wakeups*)** : Pour garantir une synchronisation robuste et empêcher l'exécution d'actions invalides lors de faux réveils, chaque thread maintient une variable locale `last_processed_tick`. Avant de procéder à ses actions, le thread valide la condition de réveil à l'aide d'une boucle `while` qui compare cette variable locale au compteur global `current_sim_tick`, s'assurant ainsi que le temps a effectivement progressé.

#### 3. Les variables volatiles

Les variables volatile sont utilisées pour indiquer les changements d'état critiques qui doivent être immédiatement visibles pour tous les threads, sans nécessiter de verrouillage explicite :
* **`alive` (Spécifique à chaque Animal)** : Indique si un animal est vivant ou mort. Lorsqu'un animal échoue à consommer, il met cette variable à `false`. Cette variable est déclarée volatile pour empêcher le compilateur de mettre en cache sa valeur dans un registre, garantissant que chaque lecture reflète l'état le plus récent en mémoire.
* **`active_simulation` (Horloge globale)** : Indique si la simulation est toujours active. Lorsque le nombre de tics maximum est atteint, l'horloge met cette variable à `false`, signalant à tous les threads de terminer leurs opérations et de se préparer à la fermeture.

## Choix de design, limitations et décisions d'implémentation

Cette section présente les décisions d'implémentation, les compromis techniques ainsi que les comportements observés lors du développement de l'écosystème, en lien avec les exigences du projet et les interprétations de l'énoncé.


### 1. Décisions d'implémentation et modélisation
* **Interprétation déterministe de la configuration des mangeoires** :
L'analyse syntaxique de la configuration de l'énoncé est définie ainsi :
```text
M(angeoire) volume (initial) [quantité (ajout) [intervalle (temps)]] (max aléatoire 100)

```


* À partir de cette structure, le choix a été fait d'écarter un mode à intervalle purement aléatoire au profit d'une approche déterministe et cyclique stricte (`counter > 0`). Si l'intervalle est omis ou égal à $0$, la mangeoire est traitée comme une ressource statique non renouvelable, ce qui simplifie la validation des scénarios limites.

* **Politique *Fail-Fast* pour la survie des animaux** :
Plutôt que de modéliser une jauge de santé dégressive inspirée d'un modèle biologique complexe, le système applique une approche *Fail-Fast*. Si un animal a faim et ne parvient pas à consommer sa portion requise lors d'un tic, son état bascule instantanément (`alive = false`) et son thread attend la fin de la simulation pour se terminer. Ce choix élimine le besoin de structures lourdes pour gérer des états de famine intermédiaires.

* **Interprétation de la ressource pendant la recharge (*Refill*)** :
L'énoncé stipulant simplement qu'un animal « meurt lorsqu’il ne peut pas se nourrir », le comportement du système lors du ravitaillement d'une mangeoire a dû être interprété. Le choix s'est porté sur la modélisation d'une indisponibilité physique stricte de la ressource : durant le temps où filling_timer == 0 (la recharge), la mangeoire agit comme un verrou logique. Tout animal tentant de consommer à ce moment précis est instantanément rejeté et subit la règle de mort immédiate (Fail-Fast), plutôt que d'être mis en attente bloquante. Ce choix permet une certain simplicité dans l'implémentation de la logique.

* **Interprétation de l'accès aux ressources par sélection pseudo-aléatoire** :
Le sujet ne précisant pas comment un animal doit cibler une mangeoire lorsqu'il y en a plusieurs, une règle d'accès a dû être interprétée. Afin d'éviter un goulot d'étranglement (phénomène de contention) systématique sur la première mangeoire disponible (index `0`), l'animal sélectionne sa mangeoire cible de façon aléatoire parmi le bassin existant à chaque fois qu'il doit se nourrir. Cette décision de conception assure une répartition homogène de la charge des threads sur l'ensemble des ressources de la simulation.

### 2. Gestion du temps et observations de l'ordonnancement


* **Utilisation de `usleep` pour la base de temps logique** :
Le rythme de l'horloge est dicté par la primitive `usleep(100000)`, fixant la durée d'un tic à 100 ms. Ce choix offre un prototype temporel simple et efficace : le temps de calcul CPU réel s'efface devant le temps logique partagé. Cela garantit que toutes les entités disposent de la même fenêtre d'action pour traiter le tic courant. Cette approche a été privilégiée pour sa simplicité dans le cadre de ce prototype

* **Impact de l'optimisation du compilateur (`-O0` vs `-O2`) et non-déterminisme** :
Lors du passage d'une compilation de débogage (`-O0`) à une compilation optimisée de production (`-O2`), des variations dans la survie de certaines entités et dans le décompte de leurs repas sont observées (par exemple, des bascules d'état VIVANT/MORT pour des acteurs d'un même scénario d'abondance).
Bien que l'outil Helgrind rapporte formellement $0$ erreur (excluant ainsi la présence de conditions de course ou *race conditions* sur la mémoire), ce comportement asynchrone demeure parfaitement normal et attendu. Les optimisations du compilateur modifient la vitesse d'exécution des instructions assembleur et influencent directement l'ordonnancement (*scheduling*) des threads par le noyau Linux. Combiné avec le choix de conception d'une sélection pseudo-aléatoire des mangeoires et du verrouillage strict lors de la recharge (*Refill*), le moment exact où un thread d'animal tente d'accéder à une mangeoire devient dépendant de cet ordonnancement.


## Jeux de tests et validation expérimentale

La validation du système repose sur un script d'automatisation (`run_tests.sh`) qui compile le projet en mode optimisé Release (`-O2`), exécute les différents scénarios et génère un rapport d'analyse de concurrence via l'outil `Valgrind --tool=helgrind`. Cette procédure permet de garantir formellement l'absence de verrous mortels (*deadlocks*) et de conditions de course (*race conditions*), l'analyseur rapportant systématiquement $0$ erreur.

| Scénario | Fichier | Tics | Comportement observé & Analyse des résultats |
| --- | --- | --- | --- |
| **Survival** *(Cas 1a)* | `scenario_survival.txt` | 15 | **4 recharges, Animal Mort.** Bien que la mangeoire se soit rechargée 4 fois, l'animal meurt après avoir consommé 2 portions. Les journaux d'exécution démontrent l'impact direct de la règle d'indisponibilité : l'animal a tenté de se nourrir au moment précis d'un ravitaillement (`filling_timer == 0`), provoquant son élimination immédiate. |
| **Extinction** *(Cas 1b)* | `scenario_extinction.txt` | 10 | **0 recharge, Animal Vivant.** La mangeoire est configurée avec un stock initial suffisant et sans mécanisme de renouvellement. L'animal consomme sa portion à 4 reprises et survit jusqu'au terme de la simulation, validant la gestion nominale d'une ressource statique non renouvelable. |
| **Abundance** *(Cas 2)* | `scenario_abundance.txt` | 30 | **10 recharges, Survie partielle.** Malgré un volume de nourriture massif et 10 recharges successives, l'Animal #1 meurt après un seul repas, tandis que d'autres survivent. L'analyse confirme que l'asynchronisme de l'ordonnanceur a poussé cet animal précis à solliciter la mangeoire au moment exact de son indisponibilité due au cycle de *Refill*. |
| **Gold Rush** *(Cas 3)* | `scenario_gold_rush.txt` | 20 | **3 recharges, Hécatombe Totale (5 Morts).** Scénario de haute contention (5 animaux pour une seule mangeoire à faible volume initial). La ressource est épuisée instantanément dès les premiers cycles. La quantité ajoutée par les recharges s'avère insuffisante pour combler la demande simultanée, provoquant une extinction rapide de tous les threads. |
| **Clan War** *(Cas 4)* | `scenario_clan_war.txt` | 25 | **4 recharges, Équilibre asymétrique.** Ce test illustre l'impact des paramètres de digestion. L'Animal #2, doté d'un cycle de digestion très court, monopolise la ressource en consommant 13 portions. Plus lent et victime d'un conflit d'accès critique lors d'une phase de recharge, l'Animal #1 se retrouve exclu et subit la règle de mort immédiate. |


## Sources consultées

* **Documentation officielle POSIX Pthreads (Linux man-pages) :**
  * `pthread_mutex_lock` / `pthread_mutex_unlock` : Gestion de l'exclusion mutuelle sur le volume des mangeoires.
  * `pthread_cond_wait` / `pthread_cond_broadcast` : Implémentation de la barrière de synchronisation de l'Horloge.
  * Référence générale : https://man7.org/linux/man-pages/man7/pthreads.7.html
* **Code des laboratoires :**
  * Compréhesttion des mécanismes de synchronisation et de la structure de base d'une application multithreadée en C++.
* **Cplusplus Reference (cppreference.com) :**
  * Bibliothèque `<random>` : Utilisation de `std::mt19937` et `std::uniform_int_distribution` combinés au qualificateur `thread_local` pour garantir un générateur de nombres aléatoires thread-safe et indépendant par animal.
* **Fils de discussion StackOverflow :**
  * *Using stdlib's rand() from multiple threads* : Étude des limitations de `rand()` en environnement concurrent et justification du passage à un générateur isolé par thread (`thread_local`).
  * Référence : https://stackoverflow.com/questions/6161322/using-stdlibs-rand-from-multiple-threads
* **Valgrind / Helgrind User Manual :**
  * Guide d'analyse et de correction des conflits d'accès mémoire (*data races*) dans les applications multithreadées.
  * **GNU Make et Bash :**
  * Utilisés pour l'automatisation de la compilation et l'exécution des jeux de test.
  * Référence Makefile : https://www.gnu.org/software/make/manual/