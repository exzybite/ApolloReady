# Origine et composants tiers

ApolloReady est un fork d’[Apollo](https://github.com/ClassicOldSong/Apollo)
0.4.6, commit `0cd32abaaa141d262477d039ac447b38fe99c394`, lui-même basé sur
[Sunshine](https://github.com/LizardByte/Sunshine). Leurs auteurs conservent tout
le crédit pour la pile de streaming et les fonctions amont.

- Le texte GPL-3.0 est conservé dans `LICENSE`.
- La présentation et les crédits d’Apollo sont conservés dans `UPSTREAM_README.md`.
- Les bibliothèques Git et leurs commits sont déclarés dans `.gitmodules` et
  `dependencies.lock.json`. Leurs fichiers de licence restent dans leurs dépôts.
- Boost 1.88.0 est récupéré par CMake avec contrôle d’intégrité.
- Les dépendances Web et leurs versions sont décrites par `package.json` et `package-lock.json`.
- Le pilote de manette [ViGEmBus](https://github.com/nefarius/ViGEmBus) 1.21.442.0
  provient de sa publication officielle ; CMake vérifie le SHA256 de l’installateur.
- SudoVDA et l’utilitaire NefConC fournis sont ceux de la
  [distribution Apollo 0.4.6](https://github.com/ClassicOldSong/Apollo/releases/tag/v0.4.6).
  Les fichiers INF, catalogue et certificat sont conservés ; le script de
  préparation vérifie les SHA256 de l’archive et des binaires extraits.

Les composants tiers conservent leurs licences et conditions respectives.
Le nom ApolloReady identifie ce fork ; il ne signifie pas que les auteurs amont
le publient, le prennent en charge ou garantissent son fonctionnement.
