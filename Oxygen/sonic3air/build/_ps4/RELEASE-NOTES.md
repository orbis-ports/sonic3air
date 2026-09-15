Sonic 3 A.I.R. (Angel Island Revisited) for the PlayStation 4, on a jailbroken console (GoldHEN).

Desktop OpenGL 4.6 through zink on RADV (mesa-ps4), SDL2 with native PS4 video, audio and controller drivers.

## Install

1. Copy the `.pkg` to the console (for example over FTP to `/data/pkg/`) and install it with
   **Settings → Debug Settings → Package Installer** (or GoldHEN's package installer).
   It installs as **Sonic 3 A.I.R.**, title id `SAIR00001`.
2. **Provide the ROM yourself - it is NOT included and never will be.** The game needs the
   *Sonic 3 & Knuckles* ROM from the Steam release (`Sonic_Knuckles_wSonic3.bin`, 4 MiB).
   Upload it over FTP to:

   ```
   /data/sonic3air/Sonic_Knuckles_wSonic3.bin
   ```

   (create the `/data/sonic3air/` directory if it does not exist; `/mnt/usb0/sonic3air/` is searched as well).
3. Optional - remastered soundtrack (~126 MB, not in the pkg): upload `audioremaster.bin` (the
   remastered soundtrack package of Sonic 3 A.I.R. - e.g. made from this repository with
   `sonic3air_linux -pack`) to `/data/sonic3air/audioremaster.bin`, then pick
   *Options → Audio → Soundtrack Type: Remastered* in the game.

Settings, progress and the log file are kept in `/data/sonic3air/savedata/`.

## Known issues

- **Closing the game from the PS4 system menu (Close Application) does not work properly** - a known
  mesa-ps4 driver bug. Quit through **Exit** in the game's main menu instead.
- Online features (update check, ghost sync, netplay) are disabled.
