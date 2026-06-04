#!/bin/bash

# Quitter immédiatement si une commande échoue (sécurité)
set -e

echo "======================================================="
echo "--- ÉTAPE 1 : COMPILATION VIA MAKEFILE ---"
echo "======================================================="

make clean
make release

if [ $? -ne 0 ]; then
    echo "Erreur lors de la compilation. Abandon."
    exit 1
fi

rm -rf test/rapports

echo "Compilation réussie avec succès (-O2)."
echo ""
echo "======================================================="
echo "--- ÉTAPE 2 : EXÉCUTION DES JEUX DE TEST ---"
echo "======================================================="

EXEC="./simulation"
TEST_DIR="test"
RAPPORT_DIR="$TEST_DIR/rapports"

# Création du dossier des rapports s'il n'existe pas
mkdir -p "$RAPPORT_DIR"


# Format : "Nom_Fichier Tics_Demandés"
SCENARIOS=(
    "scenario_survival.txt 15"
    "scenario_extinction.txt 10"
    "scenario_abundance.txt 30"
    "scenario_gold_rush.txt 20"
    "scenario_clan_war.txt 25"
)


for SCENARIO in "${SCENARIOS[@]}"; do
    # Séparation du nom de fichier et des tics
    set -- $SCENARIO
    CONFIG_FILE="$1"
    TICS="$2"
    
    PATH_CONFIG="$TEST_DIR/$CONFIG_FILE"

    if [ ! -f "$PATH_CONFIG" ]; then
        echo "[ERREUR] Le fichier $PATH_CONFIG est introuvable."
        continue
    fi

    # Extraction du nom sans extension pour les rapports
    BASE_NAME=$(basename "$CONFIG_FILE" .txt)
    OUTPUT_LOG="${RAPPORT_DIR}/${BASE_NAME}_output.log"
    HELGRIND_LOG="${RAPPORT_DIR}/${BASE_NAME}_helgrind.log"

    echo "--> Test en cours : $CONFIG_FILE pendant $TICS tics"
    
    # Exécution standard pour capturer la sortie demandée (rapport final)
    $EXEC "$PATH_CONFIG" "$TICS" > "$OUTPUT_LOG" 2>&1
    echo "    [OK] Sortie enregistrée dans : $OUTPUT_LOG"

    # Exécution avec Helgrind pour valider le multithreading
    valgrind --tool=helgrind --log-file="$HELGRIND_LOG" $EXEC "$PATH_CONFIG" "$TICS" > /dev/null 2>&1
    
    # Extraction rapide du résumé d'erreurs pour l'affichage console
    ERRORS=$(grep "ERROR SUMMARY:" "$HELGRIND_LOG" | awk '{print $4}' || echo "0")
    echo "    [OK] Rapport Helgrind : $HELGRIND_LOG (Erreurs : $ERRORS)"
    echo "-------------------------------------------------------"
done

echo "======================================================="
echo "Tous les tests sont terminés !"
echo "Les configurations restent dans : /$TEST_DIR"
echo "Les résultats sont isolés dans : /$RAPPORT_DIR"
echo "======================================================="