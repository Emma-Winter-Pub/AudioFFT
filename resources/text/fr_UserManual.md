## Espace de travail
*   **Chargement complet** : Charge le fichier audio entier en mémoire pour une vitesse de traitement maximale, adapté à l'analyse de fichiers de taille standard.
*   **Streaming** : Lit, traite et rend par blocs avec une utilisation extrêmement faible de la mémoire, idéal pour les fichiers audio très longs ou les ordinateurs à ressources limitées.
*   **Par lots** : Génère des spectrogrammes par lots, avec les deux modes intégrés Chargement complet et Streaming.

## Analyse de fichier unique
### Contrôles de vue et d'affichage
*   **Log** : Ouvre une fenêtre de journal indépendante pour afficher les informations du fichier et la progression du traitement en temps réel.
*   **Grille** : Superpose des lignes de grille de fréquence et de temps sur le spectrogramme pour faciliter l'alignement et la lecture.
*   **Labels** : Active les informations périphériques (axes, titre, etc.) autour du spectrogramme.
*   **ZoomHz** : Déverrouille le zoom de l'axe des fréquences.
*   **LMax** : Limite la largeur maximale en pixels de l'image exportée. L'image sera redimensionnée automatiquement en cas de dépassement de la limite de largeur.

### Paramètres audio et image
*   **Flux / Piste** : Change le flux audio dans les fichiers à flux multiples ; si un fichier `.cue` est chargé, cela devient **Piste** (piste CUE).
*   **Canaux** : Sélectionne un canal spécifique, ou choisissez « Mix » pour mixer tous les canaux.
*   **H** : Définit la hauteur verticale en pixels de l'image.
*   **Préc** : Définit la résolution temporelle horizontale (secondes/pixel). Lorsqu'elle est définie sur « Auto », le taux de chevauchement de la fenêtre est de 0 %.
*   **Fen** : Sélectionne la fonction de fenêtrage pour la transformée de Fourier afin de contrôler la fuite spectrale et l'atténuation des lobes secondaires.
*   **Map** : Sélectionne la courbe de mise à l'échelle pour l'axe des fréquences (axe Y) afin de se concentrer facilement sur des bandes de fréquences spécifiques.
*   **Pal** : Sélectionne le thème de couleurs pour l'énergie spectrale.
*   **dB** : Définit les limites supérieure et inférieure (dB) de l'énergie mappée.

*   **Ouvrir** : Ouvre les fichiers contenant des flux audio ainsi que les fichiers CUE.
*   **Sauver** : Exporte le spectrogramme sous forme d'image, avec un niveau de compression et une qualité d'image personnalisables.

## Traitement par lots
*   **Chemin d'entrée / Chemin de sortie** : Définit le dossier source et le dossier de destination.
*   **Paramètres** : Ouvre le centre de configuration des tâches par lots. Vous pouvez spécifier le **Mode** (Chargement complet / Streaming), les **Threads**, les paramètres FFT, le format d'exportation de l'image, la qualité de l'image et d'autres options.
*   **Contrôle des tâches** : Contrôle la tâche via les boutons **Démarrer la tâche**, **Mettre en pause la tâche / Reprendre la tâche** et **Arrêter la tâche**.