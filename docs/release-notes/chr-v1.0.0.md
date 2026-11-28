**Revenge Rebuild** is a fan rebuild of BattleTech: The Crescent Hawks'
Revenge (1990, the DOS game by Westwood Associates, published by Infocom) for
today's Windows, macOS and Linux. It is built on
[Elthial's Crescent Hawks' Revenge SDL port](https://github.com/Elthial/Crescent-Hawks-Revenge-sdl-port),
the readable reconstruction of the original program, whose research and code
it continues. It is not an emulator and not a patched copy. It is unofficial
and free, and not affiliated with or endorsed by any holder of rights in the
game.

This is the first release.

## What it is

- **The campaign and battles of the 1990 game**, rebuilt from the
  reconstruction and checked against the original running under DOSBox-X.
  Recorded runs from the title through the campaign's tenth battle match the
  original's state, and so do saves the original wrote, loaded here.
- **Music from a General MIDI soundfont**, each MT-32 sound the songs choose
  played on its nearest instrument. The settings can play it as the PC
  speaker did instead (`music = speaker`), or turn it off.
- **Effects and speech as recorded samples**, or as the PC speaker's tones
  with the speech kept clear (`sound = speaker`).
- **Your copy stays your own.** The game's words, pictures and sounds are read
  from it on the first start, and play never needs the original program
  again.
- **UPGRADES: fixes for the original's bugs, if you want them.** The game
  options menu has an UPGRADES line of ours. Its four switches are all off to
  start with, so the game plays as the original did until you turn one on.
  Each is our own fix, and each changes only your side, never the enemy's:
  - **Lance pursuit:** a lance whose leader can't walk, with no enemy in
    sight for a minute, goes after the nearest one, so a battle can't wait
    for ever.
  - **Restart pilots:** after a Restart, the lance editor deals new pilots as
    a new campaign does, instead of the same pilot over and over.
  - **Lance weapons:** your lance's computer-run 'Mechs fire the weapon each
    of their choices was meant to fire.
  - **Lance tactics:** they count their firepower right when deciding how
    close to fight.

  The page says what each one does, and they are kept with your settings.
- **The setup menu, over the game.** Shift+Alt+F1 (Cmd+Shift+F1 on a Mac), or
  the Setup menu line on the UPGRADES page, opens it while the game waits.
  It shows the game files in use and can change them; checks for updates
  and installs one (only a release whose signed checksums verify, with your
  saves and settings backed up first); reports a problem; CHECK MY FILES
  compares the program's files and your game's with the record they came
  with; and it can reset the settings or open your folder.
- **A log to send with a report.** Started from the launcher or a package,
  the game writes `revenge.log` in your folder, and keeps the one before as
  `revenge.old.log`.

## Bring your own copy of the game

None of the game's files are in these downloads. The game plays the US DOS
release 1.00, and the first start gets it ready by itself, in a small window:

- **A copy found on your computer** is checked and set up at once in the
  rebuild's own folder, with no question asked. Your copy is never changed.
  If the check finds files missing, damaged or unlike the ones this project
  knows, it shows you what is wrong first.
- **With no copy found**, the download of release 1.00 starts at once, from
  the Internet Archive, by the BattleTech launcher's own list. The list is
  used only when its signature checks, and the download only when it is
  exactly the file the list names; a download that breaks off goes on from
  where it stopped. CANCEL and USE MY OWN COPY INSTEAD are there while it
  runs.
- **If the download fails, is cancelled, or can't be had**, the setup window
  opens for you to pick your own copy: its REVENGE.EXE, any file in its
  folder, or the archive it came in.

The BattleTech launcher can install, set up and start the game too.

## Downloads

- **Windows (64-bit):** `CHR-Rebuild-chr-v1.0.0-Setup.exe` installs it with a
  Start-menu shortcut. `CHR-Rebuild-chr-v1.0.0-win64.zip` is the same program
  to unpack anywhere and run `chr.exe`. Neither is signed, so Windows may warn
  first: choose "More info", then "Run anyway".
- **macOS 12 or later (Apple silicon and Intel):**
  `CHR-Rebuild-chr-v1.0.0-mac.dmg`. Open it and drag Revenge Rebuild to
  Applications. The app is signed and notarized, so it opens like any other
  app.
- **Linux (x86-64):** `CHR-Rebuild-chr-v1.0.0-x86_64.AppImage`: make it
  executable and run it. If it doesn't start, your system may lack FUSE 2
  (the `libfuse2` package). `CHR-Rebuild-chr-v1.0.0-linux-x86_64.tar.gz` is
  the same program as a plain archive, to unpack and run as `./chr`.
- **`SHA256SUMS`** lists every file's SHA-256.

## Not yet played or heard by a person

Automated tests and recorded runs check these, but nobody has gone through
them yet:

- The whole campaign, from start to end. Recorded runs match the original
  through the tenth battle; the battles after it haven't been compared yet.
- The UPGRADES switches over a whole campaign. Each is checked by its own
  recorded run or tests.
- Updating from the setup menu. This is the first release, so no update has
  been installed from one yet; it is tested against stand-in releases.
- The Linux packages, which are built and smoke-tested automatically.
- The Intel half of the Mac package. Its checks ran under Rosetta, not on a
  real Intel Mac.
- Setting the game up on the launcher's screens, and the download on the
  setup screen. They have been tested with the launcher's own file and with a
  test list, not yet by a player.
- The effects and speech on real speakers. They were checked as recordings,
  not by ear. The music has been listened to.
- The window on a real screen: resized, maximised or in full screen.

Please report anything that looks or sounds wrong.

## Known differences from the original

- **How long a battle message stays up.** The original counted its own
  drawing passes, so on a faster or slower computer a message stayed up for a
  shorter or longer time, and a prompt could be gone before it opened. Here
  it is a fixed time of the game's clock, about 20 seconds: as long as on the
  machine the original was measured on.
- **One battle can wait for ever, as it does in the original.** In the
  campaign's eighth battle the lance's leader can be crippled where it
  stands. The other two keep formation on it, the last enemy never comes, and
  the battle doesn't end. Send one of the other two after the last enemy, but
  only once the nearer enemy is down: sent earlier, it leaves the leader
  alone and the battle is lost. This is kept as the original has it; the
  **Lance pursuit** switch on the UPGRADES page fixes it if you turn it on.
- The music plays on a General MIDI soundfont, not on an MT-32, so its
  instruments are the soundfont's.
- Only the US DOS release 1.00 is known so far. Other releases may differ.
