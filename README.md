# ApolloReady

[Français](README.fr.md)

Experimental Windows fork of [Apollo](https://github.com/ClassicOldSong/Apollo)
0.4.6, built on the [Sunshine](https://github.com/LizardByte/Sunshine) streaming stack.

**Heavily vibe-coded with AI assistance.** This is an experimental community
fork, not an official Apollo or Sunshine release.

ApolloReady adds optional server-rendered loading, closing and background-game
screens. Window-only capture keeps the desktop and launchers out of the stream
while masks are enabled. The animation runs at approximately 30 fps and follows
the language selected in the server configuration. Standard Moonlight clients
receive the masks as ordinary video frames.

## Install

1. Download the Windows x64 installer from this repository’s Releases when published.
   Check its hash against the accompanying `SHA256SUMS.txt`.
2. Close games and stop another Apollo/Sunshine server using the standard ports.
3. Run the installer. It includes the host, Web UI, service, uninstaller and required
   ViGEmBus/SudoVDA prerequisites. Working existing drivers are kept.
4. Open **https://localhost:47990**, create your credentials and pair Moonlight.
   Grant the client the launch/controller permissions it needs.
5. Enable game masks in **Configuration → General**, apply and restart the host.
6. Add a fullscreen game. Enable its virtual display and app identity options to
   stream through a dedicated display.

No existing Apollo installation is required. The Moonlight client is installed
separately on your gaming device. No developer credentials, pairings, games or
personal configuration are included. Masks are disabled by default.

## Limits and status

Window selection is a heuristic. Reused processes, windowed games, replacement
windows, rotated monitors and mismatched capture sizes may remain masked. The
launcher fallback can select another newly started fullscreen application.
Minimized or hidden games show the background mask; a still-visible background
game may continue streaming.

This preview installer is unsigned. The November 2, 2026 npm audit reported 25
dependency advisories (1 low, 8 moderate, 16 high); their relevance remains to be
triaged rather than being claimed as confirmed runtime vulnerabilities.

Updates are manual: back up `config`, run the next installer in the same location
and retain the configuration when prompted. The Windows service is
`ApolloReadyService`; Windows Installed Apps provides the uninstaller.

See [BUILDING.md](BUILDING.md), [CHANGELOG.md](CHANGELOG.md), and
[APOLLOREADY.md](APOLLOREADY.md) for build instructions and technical details.

## Credits

Apollo and Sunshine authors retain all credit for the upstream streaming stack.
The original [GPL-3.0 license](LICENSE), [upstream README](UPSTREAM_README.md),
and [third-party notices](THIRD_PARTY.md) are preserved.
