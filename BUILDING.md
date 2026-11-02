# Compilation Windows

La chaîne validée est MSYS2 UCRT64, GCC 16.2, CMake 4.4, Ninja et Node.js. Les
outils sont des dépendances de développement ; ils ne sont pas nécessaires pour
utiliser l’installateur. Installez [Git](https://git-scm.com/) et
[MSYS2](https://www.msys2.org/) puis mettez MSYS2 à jour selon sa documentation.

Depuis les sources extraites, dans PowerShell :

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\scripts\Build-Windows.ps1 -MsysRoot C:\msys64 -InstallDependencies
```

Le script télécharge les sous-modules Windows aux commits de
`dependencies.lock.json`. Il fonctionne aussi depuis l’archive de sources, sans
historique Git racine. Les autres sous-modules amont sont listés dans ce fichier
pour traçabilité. Les bibliothèques précompilées amont de `build-deps` conservent
leurs scripts de construction et notices dans leur dépôt lié.

Les deux fichiers binaires SudoVDA absents du dépôt source sont extraits de
l’archive officielle Apollo 0.4.6. Son SHA256 et ceux des fichiers extraits sont
vérifiés dans `scripts/prepare-windows-prerequisites.ps1`. Cette archive n’est
jamais exécutée : aucune installation d’Apollo existante n’est nécessaire.
ViGEmBus est téléchargé par CMake depuis sa publication officielle avec SHA256.

La version provient de `VERSION.txt`. Le script lance les tests de session et de
rendu des masques, construit le serveur et l’interface, puis produit un
installateur NSIS dans `build-gameready/cpack_artifacts`. Le `package-lock.json`
fourni correspond aux dépendances Web de cette préversion.

La compilation n’installe pas de service sur le PC. Les jeux, les identifiants,
les certificats privés et les configurations de streaming ne font pas partie
des sources de publication.

Les scripts sont actuellement validés pour Windows x64. Les fichiers hérités
des autres plateformes sont conservés pour l’historique amont, sans promesse de
compilation ou de fonctionnement d’ApolloReady sur ces plateformes.
