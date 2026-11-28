**Revenge Rebuild** is a fan rebuild of BattleTech: The Crescent Hawks'
Revenge (1990, the DOS game by Westwood Associates, published by Infocom) for
today's Windows, macOS and Linux. It is built on
[Elthial's Crescent Hawks' Revenge SDL port](https://github.com/Elthial/Crescent-Hawks-Revenge-sdl-port),
the readable reconstruction of the original program, whose research and code
it continues. It is not an emulator and not a patched copy. It is unofficial
and free, and not affiliated with or endorsed by any holder of rights in the
game.

1.0.1 fixes updating a Windows zip copy from the setup menu, and makes the
update check quieter. Nothing in the game itself has changed.

## New in 1.0.1

- **Updating a Windows zip copy works.** In 1.0.0, UPDATE NOW on a copy
  unpacked from the zip and started from its own folder (by double-clicking
  `chr.exe`, or through a shortcut) closed the game and started 1.0.0 again,
  saying the update didn't finish. The updater tried to move the game's
  folder while the folder was still in use. It now steps out of the folder
  first, and starts the updated game in it again. The Windows Setup, macOS
  and Linux copies weren't affected.
- **A quieter update check.** When the start-up check finds a new version, a
  notice shows in the corner for 15 seconds, "UPDATE AVAILABLE <version> ·
  press <keys>", and the game goes on: nothing is paused or covered. The
  setup menu's keys (Shift+Alt+F1, or Cmd+Shift+F1 on a Mac) open the update
  page with UPDATE NOW for the rest of that session. 1.0.0 showed the
  question over the game.
- **The log follows an update:** the download and where it came from, the
  signed checksums checked, the swap, and at the next start which version it
  went from and to. This helps with a report if an update goes wrong.
- **A zip copy unpacked straight onto a drive** (its files at E:\ itself, not
  in a folder) can't update itself, because a drive can't be moved aside.
  The update page now says so and offers the release page instead.

## Updating from 1.0.0

- **Windows zip copies: download 1.0.1 by hand, once.** 1.0.0's own updater
  has the folder bug above, so unpack the new zip yourself, over the old
  folder or into a new one. Your saves and settings are in your own folder,
  not the program's, so nothing is lost. Updates after 1.0.1 work from the
  setup menu.
- **Windows Setup, macOS and Linux copies** can take 1.0.1 from the setup
  menu: UPDATES, then UPDATE NOW. Your saves and settings are backed up
  first.

## Downloads

- **Windows (64-bit):** `CHR-Rebuild-chr-v1.0.1-Setup.exe` installs it with a
  Start-menu shortcut. `CHR-Rebuild-chr-v1.0.1-win64.zip` is the same program
  to unpack anywhere and run `chr.exe`. Neither is signed, so Windows may warn
  first: choose "More info", then "Run anyway".
- **macOS 12 or later (Apple silicon and Intel):**
  `CHR-Rebuild-chr-v1.0.1-mac.dmg`. Open it and drag Revenge Rebuild to
  Applications. The app is signed and notarized, so it opens like any other
  app.
- **Linux (x86-64):** `CHR-Rebuild-chr-v1.0.1-x86_64.AppImage`: make it
  executable and run it. If it doesn't start, your system may lack FUSE 2
  (the `libfuse2` package). `CHR-Rebuild-chr-v1.0.1-linux-x86_64.tar.gz` is
  the same program as a plain archive, to unpack and run as `./chr`.
- **`SHA256SUMS`** lists every file's SHA-256.

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

## Not yet played or heard by a person

Automated tests and recorded runs check these, but nobody has gone through
them yet:

- The whole campaign, from start to end. Recorded runs match the original
  through the tenth battle; the battles after it haven't been compared yet.
- The UPGRADES switches over a whole campaign. Each is checked by its own
  recorded run or tests.
- Updating this game from the setup menu. The same updater has updated the
  Inception rebuild from one release to the next, on Windows (Setup and zip)
  and on macOS; this game's own first update is from 1.0.0 to this release.
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
