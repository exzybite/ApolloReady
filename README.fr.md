# ApolloReady

[English](README.md) · [Installation](#installation) · [Notes de version](CHANGELOG.md) · [Détails techniques](APOLLOREADY.md)

ApolloReady est un fork expérimental Windows d’[Apollo](https://github.com/ClassicOldSong/Apollo), fondé sur Apollo 0.4.6 et la pile de streaming [Sunshine](https://github.com/LizardByte/Sunshine).

Il ajoute des écrans de chargement, de fermeture et de jeu en arrière-plan, générés côté serveur. La capture cible la fenêtre du jeu pour garder le bureau Windows et les lanceurs hors du flux lorsque les masques sont activés. Le client Moonlight reçoit ces écrans comme des images vidéo normales.

**Ce projet est fortement vibecodé, avec une aide importante de l’IA.** Le code est expérimental et les tests restent limités. Ce n’est pas une version officielle d’Apollo ou de Sunshine.

## Ce qu’il ajoute

- Interrupteur « Activer les masques de jeu » dans Configuration → Général.
- Masques animés à environ 30 images/s, dans la langue choisie dans la configuration.
- Capture de fenêtre, sans bascule vers une capture du bureau lorsque ce mode est actif.
- Détection du jeu directement lancé même s’il perd le premier plan pendant le démarrage.
- Reprise de la capture après minimisation et retour au jeu.
- Masque de fermeture avant le retour au client et nettoyage de l’écran virtuel après fermeture du jeu.
- Installation autonome : serveur, interface Web, service Windows et pilotes requis. Apollo n’a pas besoin d’être installé auparavant.

## Installation

1. Téléchargez l’installateur Windows x64 depuis la section **Releases** de ce dépôt lorsqu’une version est publiée. Vérifiez son SHA256 avec le fichier `SHA256SUMS.txt` de la même version.
2. Fermez vos jeux et arrêtez un éventuel autre serveur Apollo/Sunshine utilisant les ports standards.
3. Lancez l’installateur et suivez l’assistant. Les pilotes ViGEmBus et SudoVDA sont fournis ; les pilotes compatibles déjà présents sont conservés.
4. Ouvrez **https://localhost:47990**, puis créez vos propres identifiants. Le serveur apparaît sous le nom **ApolloReady**.
5. Dans Moonlight, ajoutez ce PC, effectuez l’appairage et accordez au client les permissions de lancement et de manette nécessaires.
6. Dans **Configuration → Général**, activez les masques, puis appliquez les changements et redémarrez le serveur.
7. Ajoutez un jeu dans **Applications**. Pour isoler l’affichage du PC, activez « Always create Virtual Display » et « Use App Identity » pour ce jeu. Le jeu doit être en plein écran sur l’écran diffusé.

Les masques sont désactivés par défaut. Le réglage **Langue** contrôle aussi leurs textes après application et redémarrage. La manette émulée peut être réglée sur X360 si un jeu ne répond pas au mode automatique.

L’installateur ne fournit pas le client Moonlight : installez-le séparément sur la Switch ou l’appareil de votre choix. Aucun compte, appairage, jeu ou réglage personnel n’est fourni. Les ports GameStream standards sont utilisés, dont le port Web 47990.

## Utilisation et limites

Quittez les jeux par leur menu habituel. Un jeu simplement visible en arrière-plan peut continuer à être diffusé ; le masque d’arrière-plan est utilisé quand sa fenêtre est minimisée ou masquée.

La sélection de fenêtre reste une heuristique. Les jeux qui réutilisent un processus déjà ouvert, changent de fenêtre, restent en mode fenêtré ou présentent une taille différente du flux peuvent rester masqués. Une autre application nouvellement lancée en plein écran peut aussi être sélectionnée par la stratégie de secours utilisée pour les lanceurs. Les écrans tournés et certaines transitions d’affichage ne sont pas couverts.

Un problème d’initialisation vidéo en mode headless a été signalé ; son diagnostic reste à faire.

Les tests connus portent sur GTA IV et Among Us, avec Moonlight-Switch, Joy-Con et passage dock/portable sur un PC Windows/NVIDIA. L’installation et une mise à jour ont été essayées sur ce PC. L’installation des pilotes sur un Windows totalement vierge reste à valider. Ces tests ne constituent pas une garantie de compatibilité universelle ou d’absence de flash à chaque image.

L’installateur de cette préversion n’est pas signé. L’audit npm du 2 novembre 2026 signale 25 avis sur les dépendances (1 faible, 8 modérés, 16 élevés), dont la portée reste à examiner. Ils ne sont pas présentés ici comme des vulnérabilités d’exécution confirmées.

## Mises à jour et désinstallation

Les mises à jour se font par un nouvel installateur. Sauvegardez le dossier `config` de votre installation, puis installez dans le même dossier. Conservez la configuration si l’assistant pose la question. La recherche et l’installation automatiques de mises à jour ne sont pas implémentées.

Le service s’appelle `ApolloReadyService` ; la désinstallation est disponible dans les applications Windows. Les pilotes partagés avec d’autres logiciels sont conservés par le désinstalleur.

## Compilation

Voir [BUILDING.md](BUILDING.md). Les sources et les dépendances Git sont verrouillées ; les prérequis de pilote sont extraits d’une publication officielle vérifiée par SHA256. Le script de préparation n’exécute ni n’installe Apollo d’origine.

## Crédits et licence

Tout le mérite de la pile de streaming revient aux auteurs d’**Apollo**, de **Sunshine** et de leurs dépendances. ApolloReady apporte ses changements au-dessus de cette base ; il n’en revendique pas la création.

Le code conserve sa licence [GPL-3.0](LICENSE). Les composants tiers gardent leurs licences respectives : voir [THIRD_PARTY.md](THIRD_PARTY.md) et la [présentation amont conservée](UPSTREAM_README.md). Les problèmes propres à ce fork sont à signaler sur son dépôt, avec les étapes de reproduction et des journaux expurgés.
