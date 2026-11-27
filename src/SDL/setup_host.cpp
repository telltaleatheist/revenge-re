#include "setup_host.h"

#include "Setup/FileCheck.h"
#include "Setup/Setup.h"

#include "game_paths.h"

#include <rekit/program_record.h>
#include <rekit/sdl/pickers.h>
#include <rekit/sdl/setup_screen.h>
#include <rekit/sdl/setup_serve.h>
#include <rekit/sources.h>
#include <rekit/status.h>

#include <SDL3/SDL.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#ifndef CHR_VERSION
#define CHR_VERSION "development"
#endif

namespace setup = chr::setup;

namespace {

std::string program_dir() {
  const char* base = SDL_GetBasePath();
  return base ? base : "";
}

std::string env(const char* name) {
  const char* v = std::getenv(name);
  return v ? v : "";
}

// No player at this run: a hidden or scripted one (--hidden, an offscreen or dummy video driver, the trace harness).
// No picker, message box or window that takes focus ever opens then.
bool unattended() {
  if (!env("REVENGE_HARNESS_SCRIPT").empty()) return true;
  const char* driver = SDL_GetHint(SDL_HINT_VIDEO_DRIVER);
  return driver && (!std::strcmp(driver, "offscreen") || !std::strcmp(driver, "dummy"));
}

// The game folder a start uses first: REVENGE_ASSET_DIRECTORY, else the one the setup chose.
std::string remembered(const std::string& user) {
  const std::string configured = env("REVENGE_ASSET_DIRECTORY");
  return configured.empty() ? setup::remembered_folder(user) : configured;
}

// The program's file name as the player knows it: "chr" in a package, "Revenge" in a build tree. An AppImage starts it
// through its AppRun link, which is not a name the player knows: that is chr too.
std::string program_name(const char* argv0) {
  std::string name = argv0 ? std::filesystem::path(argv0).filename().string() : "chr";
  if (name.size() > 4 && (name.ends_with(".exe") || name.ends_with(".EXE"))) name.resize(name.size() - 4);
  return name.empty() || name == "AppRun" ? "chr" : name;
}

void usage(std::FILE* to, const std::string& name) {
  std::fprintf(to,
               "Usage: %s [--windowed] [--window WxH] [--nosound] [--hidden] [--write-dir DIRECTORY] [--ready-file FILE]\n"
               "       %s --status | --setup | --version | --help\n"
               "       %s --setup --describe | --search | --copies | --look PICK | --install PICK [--folder DIRECTORY]\n"
               "Plays The Crescent Hawks' Revenge from your own copy of the DOS game. With no copy set up it opens the\n"
               "setup: pick your copy, its folder or the archive it came in, and it is copied beside the saves. --status\n"
               "prints what is found as JSON; --setup only sets the game up, with more words as the launcher asks it.\n",
               name.c_str(), name.c_str(), name.c_str());
}

// The launcher's start options (rekit::kPlayOptions).
struct PlayOptions {
  int width = 0, height = 0;  // --window WxH
  bool nosound = false, hidden = false;
  std::string write_dir, ready_file;
};
PlayOptions play;
bool window_sized = false, ready_written = false;

// "" when the arguments are the launcher's options, else what is wrong with them.
std::string parse_play(const std::vector<std::string>& args, PlayOptions* o) {
  for (size_t i = 0; i < args.size(); ++i) {
    const std::string& a = args[i];
    const auto value = [&](std::string* out) {
      if (i + 1 >= args.size() || args[i + 1].empty()) return false;
      *out = args[++i];
      return true;
    };
    if (a == "--windowed") continue;  // the game always plays in a window
    if (a == "--nosound") o->nosound = true;
    else if (a == "--hidden") o->hidden = true;
    else if (a == "--write-dir") {
      if (!value(&o->write_dir)) return "--write-dir needs a folder";
    } else if (a == "--ready-file") {
      if (!value(&o->ready_file)) return "--ready-file needs a file";
    } else if (a == "--window") {
      std::string size;
      char x = 0, rest = 0;
      if (!value(&size) || std::sscanf(size.c_str(), "%d%c%d%c", &o->width, &x, &o->height, &rest) != 3 ||
          (x != 'x' && x != 'X') || o->width < 320 || o->height < 200 || o->width > 16384 || o->height > 16384)
        return "--window needs a size, WxH, at least 320x200";
    } else
      return "unknown option " + a;
  }
  return "";
}

// The options SDL and the game read from the environment and hints, before either starts.
void apply_play(const PlayOptions& o) {
  if (!o.write_dir.empty()) SDL_setenv_unsafe("REVENGE_SAVE_DIRECTORY", o.write_dir.c_str(), 1);
  if (o.nosound) SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
  if (o.hidden) SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
  // A run no player is at never takes the foreground on macOS.
  if (unattended()) SDL_SetHint(SDL_HINT_MAC_BACKGROUND_APP, "1");
}

// The window's size and the ready file, on the game window's first showing: watched among SDL's events, so the game's
// own window code is left as it is.
bool SDLCALL watch_window(void*, SDL_Event* event) {
  if (event->type != SDL_EVENT_WINDOW_SHOWN && event->type != SDL_EVENT_WINDOW_EXPOSED) return true;
  if (play.width > 0 && !window_sized) {
    window_sized = true;  // first: setting the size sends events of its own
    if (SDL_Window* window = SDL_GetWindowFromID(event->window.windowID)) SDL_SetWindowSize(window, play.width, play.height);
  }
  if (!play.ready_file.empty() && !ready_written) {
    ready_written = true;
    std::ofstream(play.ready_file) << "ready\n";
  }
  return true;
}

void watch_for_play() {
  if (play.width <= 0 && play.ready_file.empty()) return;
  // The event queue started here stays up through the game's own SDL_Init, which counts it again.
  if (SDL_InitSubSystem(SDL_INIT_EVENTS)) SDL_AddEventWatch(watch_window, nullptr);
}

std::string json_text(const std::string& s) {
  std::string o;
  for (char c : s) {
    if (c == '"' || c == '\\') o += '\\', o += c;
    else if (static_cast<unsigned char>(c) < 0x20) {
      char b[8];
      std::snprintf(b, sizeof(b), "\\u%04x", static_cast<unsigned char>(c));
      o += b;
    } else o += c;
  }
  return o;
}

void say_error(bool setup_only, const std::string& why) {
  std::fprintf(stderr, "setup: %s\n", why.c_str());
  if (setup_only) std::printf("{\"error\": \"%s\"}\n", json_text(why).c_str()), std::fflush(stdout);
}

// A file check's one line, in the log (stderr). "" nothing was amiss; else what the player is told.
std::string say_files(const setup::FilesReport& r) {
  std::fprintf(stderr, "setup: %s\n", setup::files_log(r).c_str());
  return setup::files_note(r);
}

// The words for a find: which REVENGE.EXE it holds, and what is amiss with its files.
std::string describe(const rekit::Found& f) {
  const std::string which = f.version.empty() ? "" : " (" + f.version + ")";
  const setup::FilesReport report = setup::check_found(f);
  say_files(report);
  if (const std::string files = setup::files_pick_note(report); !files.empty())
    return std::string(setup::kTitle) + which + ". " + files;
  return std::string(f.on_disk ? "An installed copy of " : "A download of ") + setup::kTitle + which + ".";
}

// Revenge's look on the kit's screen: the rebuild's own badge (tools/package/ICONS.md; a package carries it in
// data/setup) and colours. Nothing in it is the original game's.
rekit::sdl::Theme theme() {
  rekit::sdl::Theme t;
  t.header = "BATTLETECH: THE CRESCENT HAWKS' REVENGE";
  const char* data = GamePaths_DataDirectory();
  std::error_code ec;
  if (data) {
    const std::filesystem::path badge = std::filesystem::path(data) / "setup" / "badge.png";
    if (std::filesystem::is_regular_file(badge, ec)) t.badge = badge.string();
  }
  t.bg = {10, 11, 14, 255}, t.band_colour = {26, 28, 34, 255}, t.rule = {214, 168, 74, 255};
  t.text = {233, 236, 238, 255}, t.dim = {150, 156, 164, 255}, t.accent = {214, 168, 74, 255};
  t.plate_colour = {54, 58, 66, 255}, t.plate_off = {34, 36, 42, 255};
  t.good = {214, 168, 74, 255}, t.bad = {240, 150, 130, 255};
  return t;
}

// The recipe for the kit's setup, on its screen or on the launcher's (`served`). `in_use`: the game folder in use, ""
// none.
rekit::sdl::Game kit_game(const std::string& user, const std::string& in_use, bool served) {
  rekit::sdl::Game g;
  g.id = setup::kId, g.title = setup::kTitle, g.folder_key = setup::kFolderKey, g.folder_name = setup::kFolderName;
  g.program = "REVENGE.EXE";
  g.rules = setup::rules(), g.hints = setup::hints();
  const std::string pdir = program_dir();
  g.search = [user, pdir, in_use, served] {
    bool s = false;
    if (in_use.empty()) return setup::find_install("", user, pdir, &s);
    // Choosing again: another copy than the one in use, or the player's own. But for the launcher a ready one of the
    // player's own is the answer while the one in use can't be played: its look finds that in place, and it is used.
    const std::string own = setup::default_install(user);
    if (served && setup::check_install(in_use).state != rekit::Status::kReady &&
        setup::check_install(own).state == rekit::Status::kReady)
      return own;
    return setup::find_other_install({in_use, own}, user, pdir);
  };
  g.describe = describe;
  g.recognise = setup::recognise;
  g.install = [](const rekit::Found& f, const std::string& out, const std::string&, const rekit::sdl::ProgressFn& progress) {
    const setup::Installed r = setup::install(f, out, [&](double at, const std::string& file) {
      return progress(rekit::sdl::Progress{"Copying the game files", at, file});
    });
    rekit::sdl::InstallResult o;
    o.ok = r.ok, o.cancelled = r.cancelled, o.install_dir = r.dir;
    o.summary = r.ok ? std::string(setup::kTitle) + " is set up: " + std::to_string(r.files) + " files copied into " + r.dir + "."
                : r.cancelled ? "Stopped. Nothing was changed."
                              : "Nothing was copied: " + r.error;
    if (r.ok)
      if (const std::string files = setup::files_note(setup::check_folder(r.dir)); !files.empty()) o.summary += " " + files;
    return o;
  };
  // --copies: every copy on this computer. (--add and --first-run have no hook: a DOS copy has no disc to add from,
  // and the story's import is made by the first start that plays.)
  g.copies = [user, pdir](const rekit::sdl::SetupOptions& o) { return setup::copies(o.in_use, user, pdir); };
  return g;
}

// The kit's screen: the find offered or looked for, copied in, the game's folder (or "").
std::string run_screen(bool setup_only, const std::string& found, bool searched, const std::string& user, int* exit) {
  const rekit::sdl::Game g = kit_game(user, searched ? std::string() : found, false);
  rekit::sdl::SetupOptions o;
  o.hidden = unattended();
  o.user_folder = user, o.default_out = setup::default_install(user), o.program_dir = program_dir();
  o.log = [](const std::string& line) { std::fprintf(stderr, "setup: %s\n", line.c_str()); };
  if (searched) o.look_first = found;  // found by the search: offered, to copy in or pass over
  else if (!found.empty()) {
    o.in_use = found, o.play_button = !setup_only;
    const std::string files = say_files(setup::check_folder(found));
    if (!files.empty()) o.notice = "The files in use: " + files;
    o.same_words = "Those are the files in use. Pick another copy.";
  }
  const rekit::sdl::SetupResult r = rekit::sdl::run_setup(g, theme(), o);
  *exit = r.exit;
  if (r.exit == rekit::kSetupFailed) say_error(setup_only, r.error);
  return r.exit == rekit::kSetupReady ? r.install_dir : std::string();
}

// A test's stand-in for the player's pick: walked by the kit and copied into the player's folder with no screen. ""
// with `why` when not.
std::string take_copy(const std::string& picked, const std::string& user, std::string* why) {
  const rekit::Found f = rekit::find_source(picked, setup::rules());
  std::fprintf(stderr, "setup: looked at %s: %s\n", picked.c_str(),
               f.kind == rekit::Found::kNothing ? "nothing" : f.version.c_str());
  if (f.kind != rekit::Found::kInstall) {
    *why = setup::recognise(picked);
    if (why->empty()) *why = "No copy of " + std::string(setup::kTitle) + " this program knows was found in " + picked + ".";
    return "";
  }
  say_files(setup::check_found(f));
  const setup::Installed r = setup::install(f, setup::default_install(user));
  if (!r.ok) {
    *why = r.error;
    return "";
  }
  std::fprintf(stderr, "setup: copied %d files into %s\n", r.files, r.dir.c_str());
  return r.dir;
}

// A test's hook without the test's own player's folder: why it is refused, else "".
std::string hook_without_folder() {
  if (!env("REVENGE_SAVE_DIRECTORY").empty()) return "";
  for (const char* hook : {"REVENGE_SETUP_PICK", "REKIT_SETUP_GO"})
    if (!env(hook).empty())
      return std::string(hook) + " is a test's, so it needs REVENGE_SAVE_DIRECTORY to name the test's own folder. Nothing was done.";
  return "";
}

// The served asks (rekit/sdl/setup_serve.h).
bool served_ask(const std::string& arg) {
  for (const char* ask : {"--describe", "--search", "--copies", "--look", "--install", "--add", "--first-run"})
    if (arg == ask) return true;
  return false;
}

// What is wrong with a served ask's arguments, else "". The kit passes over arguments it doesn't know, and would
// install into the default folder for a --folder without its folder.
std::string wrong_ask(const std::vector<std::string>& args) {
  const std::string& ask = args[0];
  const size_t n = args.size();
  if (ask == "--describe" || ask == "--search" || ask == "--copies") return n == 1 ? "" : "--setup " + ask + " takes nothing after it.";
  if (n < 2 || args[1].empty()) return "--setup " + ask + " needs " + (ask == "--first-run" ? "the step." : "the pick.");
  if (ask != "--install") return n == 2 ? "" : "--setup " + ask + " takes one word after it and nothing more.";
  if (n == 2 || (n == 4 && args[2] == "--folder" && !args[3].empty())) return "";
  return "--setup --install takes the pick, then --folder and a folder or nothing.";
}

// A served ask that isn't answered: its last line and exit.
int refuse_ask(const std::string& why) {
  rekit::SetupMessage m;
  m.done = true, m.summary = why;
  std::printf("%s\n", rekit::setup_message_json(m).c_str());
  std::fflush(stdout);
  std::fprintf(stderr, "setup: %s\n", why.c_str());
  return rekit::kSetupFailed;
}

// stderr of a served look or install, into setup.log in the player's folder, anew each time: the launcher shows
// nobody what is said there. Left where it is when the file can't be written.
void open_served_log(const std::string& user) {
  std::error_code ec;
  std::filesystem::create_directories(user, ec);
  const std::string path = (std::filesystem::path(user) / "setup.log").string();
  std::FILE* can = std::fopen(path.c_str(), "w");  // freopen closes a stream it can't open: see first that it can
  if (!can) return;
  std::fclose(can);
  if (std::freopen(path.c_str(), "w", stderr)) std::setvbuf(stderr, nullptr, _IONBF, 0);
}

void keep_folder(const std::string& user, const std::string& dir) {
  if (dir.empty() || setup::remembered_folder(user) == dir) return;
  if (!setup::remember_folder(user, dir))
    std::fprintf(stderr, "setup: couldn't save the game folder in %s\n", setup::kSetupFile);
}

int print_status() {
  std::fflush(stdout);
  const std::string user = setup::user_folder();
  rekit::Status st = setup::status(remembered(user), user, program_dir(), CHR_VERSION);
  // The files, for a copy that is ready and has nothing else to be said of it.
  if (st.state == rekit::Status::kReady && st.note.empty()) st.note = setup::files_status_note(setup::check_folder(st.where));
  st.setup = rekit::kServedSetup;
  st.play = rekit::kPlayOptions;
  std::printf("%s\n", rekit::status_json(st).c_str());
  std::fflush(stdout);
  return 0;
}

int serve(const std::vector<std::string>& args) {
#if !defined(_WIN32)
  std::signal(SIGPIPE, SIG_IGN);  // a launcher that has gone: the work goes on
#endif
  if (const std::string why = wrong_ask(args); !why.empty()) return refuse_ask(why);
  if (const std::string why = hook_without_folder(); !why.empty()) return refuse_ask(why);
  const std::string user = setup::user_folder();
  if (user.empty()) return refuse_ask("There is no folder for the player's files.");
  try {
    if (args[0] == "--look" || args[0] == "--install") open_served_log(user);
    const std::string first = remembered(user);
    std::string in_use;
    if (args[0] == "--search" || args[0] == "--copies") {
      bool searched = false;
      in_use = setup::find_install(first, user, program_dir(), &searched);
      if (searched) in_use.clear();
    }
    rekit::sdl::SetupOptions o;
    o.hidden = true, o.user_folder = user, o.default_out = setup::default_install(user), o.program_dir = program_dir();
    o.in_use = args[0] == "--copies" ? in_use : std::string();
    o.log = [](const std::string& line) { std::fprintf(stderr, "setup: %s\n", line.c_str()); };
    const rekit::sdl::SetupResult r =
        rekit::sdl::serve_setup(kit_game(user, args[0] == "--search" ? in_use : std::string(), true), o, args);
    keep_folder(user, r.install_dir);
    return r.exit;
  } catch (const std::exception& e) {
    return refuse_ask(std::string("The setup failed: ") + e.what());
  }
}

// The game folder this start plays from, into `folder`: the remembered one, the player's own install, else the setup
// (the player picks a copy, a folder or an archive, which is copied into the player's folder). `setup_only`: the
// setup whatever is found (--setup). Returns the launcher contract's exit.
int resolve_game(bool setup_only, std::string* folder) {
  rekit::sdl::unattended() = [] { return unattended(); };
  if (const std::string why = hook_without_folder(); !why.empty()) return say_error(setup_only, why), rekit::kSetupFailed;
  const std::string user = setup::user_folder();
  bool searched = false;
  const std::string found = setup::find_install(remembered(user), user, program_dir(), &searched);
  std::string game;
  if (!setup_only && !found.empty() && !searched) game = found;
  if (const std::string scripted = env("REVENGE_SETUP_PICK"); game.empty() && !scripted.empty()) {
    std::string why;
    game = take_copy(scripted, user, &why);
    if (game.empty()) return say_error(setup_only, why), rekit::kSetupFailed;
  }
  // A check's hidden screen (REKIT_SETUP_FRAMES, the kit's hooks driving it; still no picker), else none unattended.
  if (game.empty() && unattended() && env("REKIT_SETUP_FRAMES").empty()) {
    say_error(setup_only, "No copy of " + std::string(setup::kTitle) + " is set up, and no player is at this run.");
    return rekit::kSetupFailed;
  }
  if (game.empty()) {  // a player is here (or a check): the kit's setup screen
    int exit = rekit::kSetupCancelled;
    game = run_screen(setup_only, found, searched, user, &exit);
    if (game.empty()) {
      if (exit == rekit::kSetupCancelled) std::fprintf(stderr, "setup: cancelled\n");
      return exit;
    }
  }
  keep_folder(user, game);
  std::fprintf(stderr, "setup: game files %s\n", game.c_str());
  *folder = game;
  return rekit::kSetupReady;
}

// A released build's start with a player at it says where this program is, for the BattleTech launcher.
void record_program() {
  const char* tag = setup::build_tag();
  if (!*tag || unattended()) return;
  rekit::record_program(setup::user_folder(), setup::kId, tag);
}

// Whether a start already knows its game folder without the setup: REVENGE_ASSET_DIRECTORY, or REVENGE.CFG beside
// the program (src/SDL/host_config.cpp reads them).
bool folder_configured() {
  if (!env("REVENGE_ASSET_DIRECTORY").empty()) return true;
  std::error_code ec;
  return std::filesystem::is_regular_file(std::filesystem::path(program_dir()) / "REVENGE.CFG", ec);
}

}  // namespace

int SetupHost_Start(int argc, char** argv) try {
  const std::vector<std::string> args(argv + (argc > 0 ? 1 : 0), argv + argc);
  const std::string name = program_name(argc > 0 ? argv[0] : nullptr);
  // The launcher's asks about the setup, answered on stdout alone, before anything else of a start.
  if (args.size() >= 2 && args[0] == "--setup" && served_ask(args[1])) return serve({args.begin() + 1, args.end()});
  if (args.size() == 1 && args[0] == "--status") return print_status();
  if (args.size() == 1 && (args[0] == "--help" || args[0] == "-h")) return usage(stdout, name), 0;
  const bool setup_only = args.size() == 1 && args[0] == "--setup";
  if (!setup_only) {
    if (const std::string why = parse_play(args, &play); !why.empty()) {
      std::fprintf(stderr, "%s: %s\n", name.c_str(), why.c_str());
      usage(stderr, name);
      return 2;
    }
  }
  apply_play(play);
  if (!setup_only && folder_configured()) {
    record_program();
    watch_for_play();
    return -1;
  }
  std::string folder;
  const int ready = resolve_game(setup_only, &folder);
  if (setup_only) return ready;
  if (ready != rekit::kSetupReady) return ready == rekit::kSetupCancelled ? 0 : 1;  // the player's own cancel ends quietly
  SDL_setenv_unsafe("REVENGE_ASSET_DIRECTORY", folder.c_str(), 1);
  record_program();
  watch_for_play();
  return -1;
} catch (const std::exception& e) {
  std::fprintf(stderr, "setup: %s\n", e.what());
  return 1;
}
