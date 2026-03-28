### Journal des modifications

---
**V1.1    20260328**

**Nouveautés**
*   Ajout du traitement en streaming.
*   Ajout du décodage multithread pour les formats FLAC, ALAC et DSD.
*   Ajout de la précision de calcul en virgule flottante adaptative 32/64 bits.
*   Ajout de la stratégie de chargement dynamique de la mémoire en mode complet.
*   Ajout du changement de piste.
*   Ajout de la prise en charge de l’ouverture des fichiers CUE.
*   Ajout du changement de piste fractionnée CUE.
*   Ajout du changement de canal.
*   Ajout de la sélection de la fonction de fenêtre FFT.
*   Ajout de la sélection du schéma de couleurs du spectrogramme.
*   Ajout de l’ajustement de la valeur dB du spectrogramme.
*   Ajout du mécanisme de mise en cache des résultats de calcul de la transformée de Fourier.
*   Ajout du rappel de tâche en double pour le traitement par lots.
*   Ajout du lecteur avec compensation de latence.
*   Ajout du curseur en croix réglable.
*   Ajout de la sonde avec source de données commutable.
*   Ajout de l’affichage du graphique de distribution des fréquences.
*   Ajout de l’accélération matérielle GPU.
*   Ajout du contrôle d’affichage/masquage des composants.
*   Ajout de l’ajustement du taux de rafraîchissement.
*   Ajout de la planification des E/S pour le traitement par lots.
*   Ajout de la fonctionnalité de capture d’écran.
*   Ajout du panneau de paramètres.
*   Ajout de la sauvegarde de la configuration utilisateur.
*   Ajout de la prise en charge multilingue : chinois simplifié, chinois traditionnel, japonais, coréen, allemand, anglais, français et russe.
*   Extension de la plage des valeurs de hauteur et ajout des valeurs de résolution point à point FFT d’origine.
*   Extension de la plage des valeurs de précision temporelle et ajout du taux de chevauchement zéro automatique.
*   Extension du nombre de fonctions de mappage.

**Optimisations**
*   Optimisation de la vitesse de décodage audio.
*   Optimisation de la vitesse de la transformée de Fourier.
*   Optimisation de la vitesse de rendu du spectrogramme.
*   Optimisation du contenu et de la disposition du journal.
*   Optimisation de la logique et de la fluidité du zoom et du déplacement du spectrogramme.
*   Changement de l’interface utilisateur vers le style Ribbon.

**Corrections**
*   Correction des erreurs de décodage multithread pour le format APE.
*   Correction de l’affichage imprécis de la durée audio pour certains fichiers.
*   Correction des fuites de ressources FFmpeg.
*   Correction des plantages du programme causés par la contention de threads.
*   Correction des plantages du programme causés par la transformée de Fourier pendant le traitement par lots.
*   Correction des échecs d’enregistrement lors du traitement par lots lorsque la taille de l’image dépassait les limites du format.

---
**V1.0    20251221**

*   Prend en charge deux modes de fonctionnement : fichier unique et traitement par lots.
*   Prend en charge la grande majorité des formats audio courants.
*   Le spectrogramme prend en charge le déplacement et le zoom.
*   Présélection de plusieurs fonctions de mappage de fréquences.
*   La hauteur du spectrogramme et la précision temporelle peuvent être ajustées.
*   Fournit une grille pour un alignement et une visualisation faciles.
*   Prend en charge l’exportation vers plusieurs formats d’image.
*   Les images exportées permettent l’ajustement de la qualité et du taux de compression.
*   Prend en charge la largeur d’image maximale personnalisée.
*   Fournit l’affichage du journal.