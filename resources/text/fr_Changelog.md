### Journal des modifications

---
**V1.3    20260725**

**Nouveautés**
*   Ajout de plusieurs fonctions de fenêtre FFT.
*   Ajout de plusieurs palettes de couleurs dBFS.
*   Ajout d'une option pour inverser la direction de la palette de couleurs.
*   Ajout d'une option pour inverser les couleurs (couleurs négatives).
*   Ajout du format de couleur indexée pour les images PNG et BMP.
*   Ajout de la prise en charge des palettes de couleurs personnalisées définies par l'utilisateur.
*   Ajout des contrôles multimédias du système pour Windows 10/11.
*   Ajout des contrôles multimédias du système pour Linux.
*   Ajout d'un module d'analyse de la structure de stockage pour Linux.
*   Ajout de l'installation du lanceur « .desktop » pour Linux.
*   Ajout de la détection de l'encodage des caractères.
*   Ajout de l'extension automatique du spectrogramme lors de la lecture.

**Optimisations**
*   Refactorisation du module de palette de couleurs.
*   Ajustement de certains paramètres par défaut.
*   Ajustement des propriétés d'empilement des fenêtres (window stacking) sous Linux.
*   Ajustement de la logique d'association des fichiers audio pour les fichiers CUE.
*   Ajustement des configurations de compilation (build) pour la compatibilité avec les compilateurs MSVC et GCC.
*   Découplage du cycle de vie de l'opération d'arrêt du lecteur et de la réinitialisation de la progression.
*   Ajout de routines globales de nettoyage des ressources FFTW.
*   Ajout de mécanismes explicites de libération de mémoire pour la plate-forme Linux.
*   Optimisation de l'analyse des chemins de fichiers externes sous Linux.
*   Optimisation de l'algorithme d'exportation libpng.
*   Optimisation de l'algorithme FFT.
*   Optimisation de l'algorithme de mappage du domaine fréquentiel.
*   Optimisation de l'ordre d'imbrication des boucles pour le rendu du spectrogramme.
*   Optimisation de la gestion de la mémoire du rééchantillonneur audio (resampler).
*   Optimisation de la logique de pré-allocation de mémoire pour le décodage multithread.
*   Optimisation du transfert de données en mode de rendu CPU.
*   Optimisation des mises à jour du tampon de sommets (vertex buffer) du GPU.
*   Optimisation de la logique de construction des sommets dynamiques sur le GPU.

**Corrections**
*   Correction d'un problème où le profil spectral dessinait toujours la première image.
*   Correction d'un problème où le lecteur arrêtait inopinément la lecture.
*   Correction d'un problème où le cache FFT ne pouvait pas se vider automatiquement.
*   Correction d'un risque de pic de mémoire lors de la fusion des données dans le décodage multithread.
*   Correction d'une erreur de mémoire non alignée dans le décodeur FFmpeg.
*   Correction des conditions de concurrence (data races) et des plantages du programme liés à FFTW.
*   Correction d'une fuite de mémoire dans l'encodeur PNG.
*   Correction de plantages incontrôlables dans le rappel d'erreur (error callback) PNG.
*   Correction d'un risque de plantage lié à des pointeurs pendants (dangling pointers) dans la fonction de capture d'écran.
*   Correction d'une erreur dans la logique d'évaluation COM au sein du module d'analyse de stockage.
*   Correction d'une erreur de division par zéro dans les fonctions de fenêtre.
*   Correction d'un problème où le module de rendu GPU plantait dans des conditions extrêmes.
*   Correction des échecs de compilation causés par des incompatibilités multiplateformes.
*   Correction d'un problème de déchirure visuelle (tearing) avec la tête de lecture.
*   Correction des problèmes de désynchronisation audio-visuelle dans le lecteur.
*   Correction d'un problème où les captures d'écran étaient capturées avec un décalage visuel.
*   Correction des anomalies de reconnaissance des points d'ancrage dans le décodage parallèle APE.
*   Correction d'une vulnérabilité où le traitement par lots pouvait déclencher une boucle infinie pour les fichiers APE.
*   Correction d'un problème où les journaux étaient omis lors du traitement par lots.
*   Correction d'un problème de blocage de lecture lors du changement d'espace de travail sous Linux.
*   Correction d'un interblocage (deadlock) permanent lorsque des exceptions se produisaient lors du traitement par lots.
*   Correction de plantages dus à des violations d'accès mémoire lors de l'arrêt des tâches ou de la fermeture de l'application.

---
**V1.2    20260610**

**Nouveautés**
*   Ajout d'un module d'analyse des périphériques de stockage.
*   Ajout d'une vue en liste virtualisée pour les journaux.
*   Ajout de notifications périodiques pendant la phase d'analyse (scan) du traitement par lots.
*   Ajout d'un mécanisme de filtrage par extension de fichier pour les analyses de traitement par lots.
*   Ajout de la détection et de la journalisation des fichiers anormaux lors du traitement par lots.
*   Ajout d'une option pour exclure les fichiers vidéo lors du traitement par lots.
*   Ajout d'une option pour catégoriser la sortie par type d'encodage lors du traitement par lots.
*   Ajout d'une orientation horizontale pour la disposition du profil spectral.
*   Ajout de la synchronisation de la fréquence d'images entre le profil spectral et la tête de lecture.
*   Ajout d'une option pour autoriser l'exécution simultanée de plusieurs instances.
*   Ajout d'une option de lecture automatique lors de l'ouverture de fichiers via les associations de fichiers du système d'exploitation.
*   Ajout d'une option pour sélectionner l'espace de travail par défaut au démarrage.

**Optimisations**
*   Optimisation de la précision et de la tolérance aux pannes de l'analyseur (parser) CUE.
*   Optimisation de la fluidité de rendu de la fenêtre des journaux.
*   Optimisation du mécanisme sous-jacent de synchronisation temporelle du lecteur audio.
*   Optimisation de la stratégie de gestion des tâches asynchrones en arrière-plan et du cycle de vie des threads.
*   Optimisation des vérifications de sécurité des allocations mémoire lors du rendu du spectrogramme.
*   Optimisation du moment de déclenchement de l'analyse des périphériques de stockage.
*   Optimisation de la journalisation des paramètres pour le traitement par lots.

**Corrections**
*   Correction d'un problème d'affichage du numéro de la piste audio.
*   Correction d'erreurs d'analyse des fichiers CUE.
*   Correction d'un problème où les données résiduelles n'étaient pas nettoyées lors d'un échec du pipeline de décodage dédié.
*   Correction d'une fuite de mémoire potentielle dans l'encodeur d'images JPEG.
*   Correction d'une violation d'accès à l'API sous-jacente lorsque le périphérique de sortie audio est débranché pendant la lecture.
*   Correction d'une boucle infinie de réponse de la disposition (layout) qui se produisait lorsque la taille de la fenêtre ou des contrôles ne changeait pas réellement.
*   Correction de problèmes liés au cycle de vie et à la synchronisation de la contre-pression (backpressure) dans la file d'attente d'écriture asynchrone du traitement par lots.
*   Correction d'un problème où la transformée de Fourier unilatérale manquait de compensation d'énergie bilatérale, entraînant des calculs d'énergie spectrale globalement plus faibles.
*   Correction d'une grave fuite de mémoire et d'un problème de double libération (double-free) causés par un nettoyage incorrect des flux d'E/S personnalisés de FFmpeg.
*   Correction d'un plantage causé par des threads asynchrones en arrière-plan capturant des pointeurs pendants (dangling pointers) lors du basculement ou de la fermeture de fenêtres pendant l'exportation d'images.
*   Correction d'un échec de sauvegarde lors de l'exportation aux formats TIFF et JPEG 2000 sous Windows, dû à un manque de prise en charge des chemins Unicode.
*   Correction d'un plantage potentiel lié à un accès mémoire hors limites (out-of-bounds) sous Windows lors de la prise de captures d'écran incluant le curseur de la souris, causé par l'absence de validation des valeurs de retour de l'API sous-jacente.
*   Correction d'un problème où la superposition du spectre pouvait s'afficher de manière incorrecte avec l'accélération matérielle du GPU en raison d'un mauvais timing lors de l'initialisation du format du contexte OpenGL.
*   Correction d'une corruption de la mémoire tas (heap) et de plantages de l'application causés par l'accès à des pointeurs pendants d'objets détruits lors du nettoyage des tâches de traitement par lots.
*   Correction d'une fuite de mémoire lors de l'initialisation du thread d'E/S.
*   Correction d'une erreur de logique séquentielle concernant la désactivation des boutons de l'interface utilisateur pendant le traitement par lots.
*   Correction d'un problème empêchant de mettre en pause, de reprendre ou de terminer les tâches de traitement par lots pendant la phase d'analyse.
*   Correction d'erreurs de calcul du chemin de sortie lors du traitement par lots.
*   Correction d'un problème où la vue du traitement par lots répondait de manière incorrecte à la barre d'espace.

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
**V1.0    20251104**（bilibili）

**V1.0    20251221**（GitHub）

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