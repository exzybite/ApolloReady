# Notes de version

## 0.1.0-preview.1

Première préversion Windows d’ApolloReady, basée sur Apollo 0.4.6
(`0cd32abaaa141d262477d039ac447b38fe99c394`).

- Masques de chargement, fermeture et jeu en arrière-plan rendus côté serveur.
- Animation à environ 30 images/s et textes liés à la langue de la configuration.
- Traductions couvrant les 20 codes de langue proposés (18 traductions distinctes).
- Sélection des fenêtres du processus lancé même sans premier plan initial.
- Reprise de capture après minimisation et fermeture normale avec nettoyage de l’écran virtuel.
- Interrupteur des masques dans l’interface Web.
- Installateur autonome, service ApolloReadyService, ports standards et désinstalleur.
- Prérequis ViGEmBus/SudoVDA inclus ; pilotes compatibles déjà présents conservés.

Validation fonctionnelle connue : GTA IV, Among Us, Moonlight-Switch, commandes
Joy-Con, dock/portable et sortie par le menu sur une configuration Windows/NVIDIA.
Les tests automatisés de session et de rendu passent. La préversion ne garantit
pas la compatibilité de tous les jeux. L’installation des pilotes sur une machine
Windows totalement vierge n’a pas encore été validée.

Voir le README pour les limites et l’état de l’audit des dépendances Web.

Signalement en attente : échec d’initialisation capture/encodage en mode headless.
