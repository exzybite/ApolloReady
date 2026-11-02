# ApolloReady (experimental Windows fork)

ApolloReady is the streaming host being developed from Apollo 0.4.6,
commit `0cd32abaaa141d262477d039ac447b38fe99c394`. It is self-contained. Original Apollo/Sunshine licenses,
notices and credits are retained. No upstream release is advertised as an
ApolloReady update.

## Window-only backend

The opt-in setting `apolloready_masks = enabled` is exposed in Configuration >
General. Apply the change and restart the host. Enabled masks force window-only
WGC regardless of the saved capture preference. Disabling masks restores the
usual capture selection, which may show the desktop. The former experimental
`capture = wgc-game` setting migrates to masks enabled unless an explicit toggle
overrides it. New installations default to masks disabled. Building the fork
does not alter an installed configuration.

The experimental backend starts with a locally generated dark texture and
never opens a monitor capture session. It first looks for a fullscreen window
owned by the process launched for the current session, even if that window lost
foreground during stream initialization. Launchers without such a window retain
the new-foreground-window heuristic. The window must be on the streamed monitor,
stable for 300 ms, and owned by a process created after the launch began. It captures that
specific HWND with Windows.Graphics.Capture. Only a valid window texture of
the expected size/format can replace the mask before encoding. A static game
image can be repeated. A minimized/invisible source is masked without ending
the session. Destroyed windows, ended processes and capture-item closure
latch `closing` until a new application launch. Capture reinitialization does
not discover a replacement app in that session.

The selected PID, process creation time and launch generation prevent a stale
capture from changing a newer session. They do not prove that a fullscreen
process is the intended game. Selection remains a heuristic: a different new
fullscreen application can be selected. Preexisting/reused processes, windowed
games, rotated monitors and textures that do not exactly match the output
dimensions are not supported by this first version. Such situations remain
masked. The backend does not change the game's display mode or inject code.

Loading, closing and background-game screens are rendered offscreen by the
host and uploaded into the encoder texture. The language selected in Configuration
is also used for loading, closing and background-game labels. All current Web UI
locales are covered, including Brazilian Portuguese and traditional Chinese;
unknown language codes fall back to English. Apply settings and restart the host
to change the language. Screens include an animated indeterminate bar. SDR BGRA and linear scRGB HDR
texture formats are handled. No desktop pixels are used for these screens.
The closing screen is allowed 1.5 seconds before the standard GameStream
termination message. This requires no auxiliary server or modified Moonlight
client. The status API remains optional for diagnostics and future clients.
On November 2, 2026, the loading and background screens were observed via
SysDVR on Moonlight-Switch. Normal exit through GTA IV's Pause > Partie >
QUITTER > confirmation screen showed the closing mask and returned to the
Moonlight library. The encoder stayed alive for approximately 1.53 seconds
after the source closed. The Web UI toggle and capture-selector locking were
also checked, with masks left enabled. This is one Windows/NVIDIA setup, not
a frame-by-frame guarantee or a universal compatibility result. A GTAIV.exe
0xc0000005 startup crash occurred once; later launches succeeded with the
same mod configuration. Its cause is unresolved.

Alt+Tab recovery: window mode bypasses the Desktop Duplication permission
probe during monitor enumeration/initialization. Exclusive fullscreen can
deny that unrelated API and otherwise cause the game's output to disappear
from capture selection. After observing a minimized/hidden source, restore
reopens WGC for the same bound HWND, with the existing process/generation
checks. No replacement application or monitor capture is opened. This fix
was confirmed by the user in the earlier dark-mask build on GTA IV. The new
rendered-mask version still needs the same regression check.

## Paired-client status API

Windows HTTPS endpoint: `/apolloreadystatus?appid=<Apollo application ID>`.
It uses the existing GameStream TLS port, paired-client certificate validation
and stream-view permissions. It has no unencrypted HTTP equivalent and needs no additional key or port.

The response follows the existing XML root/status_code convention. With the
backend enabled and a matching session it includes `appid`, `generation`,
`enabled`, `status` (`waiting`, `ready`, `closing`, `idle`) and numeric `ready`.
A missing/mismatched AppID returns XML status code 409 and `unknown_session`.
An inactive backend returns `disabled`. No local executable path or window
title is returned. Clients must reject responses from a superseded generation.
Standard Moonlight does not need to call this endpoint to display the masks.

## Validation and deployment

`tests/apolloready_session_test.cpp` verifies old-process rejection, source
locking, the closing latch, AppID/generation reset and concurrent stale state
updates. The development build script runs it before building the server.
This is not an integration test of WGC, TLS or the encoder.

`tests/apolloready_mask_test.cpp` renders three screens and checks dimensions,
opacity, animation changes and HDR channel conversion, and writes BMP previews.
`scripts/build-windows.sh` runs this test before building and packaging the host.

Mask animation is refreshed at approximately 30 fps. Its three-second, eased
round trip follows a monotonic clock, so delayed frames do not slow the animation.
Among Us loading and normal menu exit, including the smoother animation, were confirmed on Switch without an auxiliary server. The GTA IV launcher now starts Steam directly and waits for the game to close; its updated launch path still needs a live check.

Before live use: build the native binary and Web UI; stage required DLLs/assets
and an isolated configuration; verify permissions and capture behavior in a
real stream using a standard Moonlight client. The installed Apollo service,
a working Moonlight client must remain available for rollback. Do not run
two host instances on the same ports. Service names and installer firewall
rules in this fork are distinct; its scripts do not remove upstream services.
No service/installer scripts are executed by the development build.
