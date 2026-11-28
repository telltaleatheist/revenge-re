#include "kit_menu.h"

#include "game_paths.h"
#include "harness.h"
#include "settings.h"
#include "setup_host.h"
#include "Setup/FileCheck.h"
#include "Setup/Setup.h"
#include "revenge/platform_backend.h"
#include "revenge/upgrades.h"

#include <rekit/program_record.h>
#include <rekit/sdl/game_menu.h>
#include <rekit/sdl/update.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#ifndef CHR_VERSION
#define CHR_VERSION ""
#endif

namespace fs = std::filesystem;
namespace setup = chr::setup;

namespace
{

std::unique_ptr<rekit::sdl::GameMenu> menu;
void (*deliverToGame)(const SDL_Event *);

// The kit's picture, copied as it is drawn (its pixels are the kit's only until the next frame), and the texture made
// from it on the game's renderer.
std::vector<unsigned char> pixels;
int pictureW, pictureH;
uint64_t pictureSerial, uploadedSerial;
bool drawnThisFrame, showing;
SDL_Texture *overlay;
uint64_t lastFrameMS;

// A path from UTF-8 (fs::u8path is deprecated in C++20).
fs::path utf8_path(const std::string &s)
{
    return fs::path(std::u8string(s.begin(), s.end()));
}

std::string env(const char *name)
{
    const char *v = SDL_getenv(name);
    return v ? v : "";
}

std::string with_tag(const char *pattern)
{
    return std::string("CHR-Rebuild-{tag}-") + pattern;
}

// How this program was installed, from where it runs, and the release file an update installs from (the packages
// tools/build_package.py makes). A build in its tree, or one with no release tag, is never updated.
rekit::InstallKind install_kind(std::string *file)
{
    std::error_code ec;
    const fs::path self = utf8_path(rekit::self_path());
    const fs::path dir = self.parent_path();
    if (!*setup::build_tag() || self.empty() || fs::is_regular_file(dir / "CMakeCache.txt", ec) ||
        fs::is_regular_file(dir.parent_path() / "CMakeCache.txt", ec))
        return rekit::InstallKind::kDevelopment;
#if defined(__APPLE__)
    if (self.generic_string().find(".app/Contents/MacOS/") != std::string::npos)
        return *file = with_tag("mac.dmg"), rekit::InstallKind::kMacApp;
    return rekit::InstallKind::kUnknown;
#elif defined(_WIN32)
    // Inno Setup's uninstaller beside the program: the Setup's install, for this user when it is in this user's own
    // programs folder (%LOCALAPPDATA%\Programs, the Setup's choice "only for me"), else for every user.
    if (fs::is_regular_file(dir / "unins000.exe", ec))
    {
        *file = with_tag("Setup.exe");
        const std::string local = env("LOCALAPPDATA");
        const fs::path mine = local.empty() ? fs::path() : utf8_path(local) / "Programs";
        const std::string at = dir.lexically_normal().generic_string(), base = mine.lexically_normal().generic_string();
        const bool own = !base.empty() && at.size() > base.size() && _strnicmp(at.c_str(), base.c_str(), base.size()) == 0;
        return own ? rekit::InstallKind::kWindowsSetupUser : rekit::InstallKind::kWindowsSetupMachine;
    }
    return *file = with_tag("win64.zip"), rekit::InstallKind::kFolder;
#else
    if (!env("APPIMAGE").empty()) return *file = with_tag("x86_64.AppImage"), rekit::InstallKind::kAppImage;
    return *file = with_tag("linux-x86_64.tar.gz"), rekit::InstallKind::kFolder;
#endif
}

// The folder whose rebuild-files.sha256 records the program (tools/files_manifest.py, run by the packaging): beside
// the program; in a Mac app its Resources (SDL's base path there), as a plain file at the top of Contents breaks the
// signature; in an AppImage the AppDir.
std::string program_folder()
{
    if (const std::string appdir = env("APPDIR"); !env("APPIMAGE").empty() && !appdir.empty()) return appdir;
    const char *base = SDL_GetBasePath();
    return base ? base : "";
}

std::string in_folder(const char *folder, const char *name)
{
    return folder ? (utf8_path(folder) / name).string() : std::string();
}

// The UPGRADES entry: picked, the page opens at the battle loop's next key check (BattleRuntime.cpp); elsewhere it is
// dimmed, with the note that says where the page is.
rekit::sdl::MenuEntry upgrades_entry()
{
    rekit::sdl::MenuEntry e;
    e.label = Upgrades_Text(UPGRADES_TEXT_OPTIONS_LINE);
    e.pick = []
    { Upgrades_RequestPage(); };
    e.available = []
    { return Upgrades_PageAvailable() != 0; };
    e.unavailable_note = Upgrades_Text(UPGRADES_TEXT_ELSEWHERE);
    return e;
}

rekit::sdl::MenuHooks hooks(SDL_Window *window)
{
    rekit::sdl::MenuHooks h;
    const std::string user = setup::user_folder();
    const char *write = GamePaths_WriteDirectory();
    h.window = window;
    h.draw = [](const rekit::sdl::MenuImage &image)
    {
        drawnThisFrame = true;
        if (image.serial == pictureSerial && pictureW == image.w && pictureH == image.h && !pixels.empty()) return;
        pictureW = image.w, pictureH = image.h, pictureSerial = image.serial;
        pixels.resize(size_t(image.w) * size_t(image.h) * 4);
        const unsigned char *from = static_cast<const unsigned char *>(image.pixels);
        for (int row = 0; row < image.h; ++row)
            std::memcpy(&pixels[size_t(row) * size_t(image.w) * 4], from + size_t(row) * size_t(image.pitch), size_t(image.w) * 4);
    };
    h.deliver = [](const SDL_Event &e)
    {
        if (deliverToGame) deliverToGame(&e);
    };
    h.pause = []
    {
        HostClock_Pause();
        SDLBackend_PauseAudio();
    };
    h.resume = []
    {
        HostClock_Resume();
        SDLBackend_ResumeAudio();
    };
    h.title = setup::kTitle;
    h.version = CHR_VERSION;
    h.user_folder = user;
    h.log_file = SetupHost_LogFile();
    h.files_in_use = env("REVENGE_ASSET_DIRECTORY");
    h.report_url = "https://github.com/telltaleatheist/revenge-re/issues";
    h.update.recipe.key = setup::kId;
    h.update.recipe.repo = "revenge-re";
    h.update.recipe.tag_prefix = "chr-v";
    h.update.recipe.tag = setup::build_tag();
    h.install = install_kind(&h.update.recipe.file);
    if (h.install == rekit::InstallKind::kDevelopment || h.install == rekit::InstallKind::kUnknown)
        h.update.recipe.repo.clear();  // nothing to update: no check
    h.update.catalogs.assign(std::begin(setup::kOriginalCatalogs), std::end(setup::kOriginalCatalogs));
    h.update.keys = { setup::kCatalogKey };
    // What an update backs up first: the settings and the saves, in our write folder.
    h.update.keep = { in_folder(write, HOST_SETTINGS_FILE_NAME), in_folder(write, "SAVEGAME.DAT") };
    h.quit = []
    { Platform_RequestQuit(); };
    h.program_dir = program_folder();
    // Inno Setup's uninstaller comes with the install, after the record was made.
    h.leave_out = { { "unins000.exe", "" }, { "unins000.dat", "" } };
    // The game folder is only read: everything the game writes goes to our write folder (game_paths.h), so nothing
    // in it is rewritten.
    h.rewritten = {};
    h.check_game_files = []
    {
        std::vector<std::string> problems;
        const std::string folder = env("REVENGE_ASSET_DIRECTORY");
        if (setup::check_install(folder).state != rekit::Status::kReady)
            return std::vector<std::string>{ "The game folder holds no copy of " + std::string(setup::kTitle) + " this program can play." };
        if (const std::string note = setup::files_note(setup::check_folder(folder)); !note.empty()) problems.push_back(note);
        return problems;
    };
    h.settings = { in_folder(write, HOST_SETTINGS_FILE_NAME) };
    h.reset_settings = []
    { return HostSettings_Reset() != 0; };
    h.restart_for_setup = []
    {
        if (!rekit::sdl::relaunch_for_setup({ "--setup" }))
            std::fprintf(stderr, "setup menu: the setup could not be started again\n");
        Platform_RequestQuit();
    };
    h.entries.push_back(upgrades_entry());
    h.log = [](const std::string &line)
    { std::fprintf(stderr, "setup menu: %s\n", line.c_str()); };
    return h;
}

void open_setup_menu()
{
    KitMenu_Show();
}

}  // namespace

const PlatformMenu *KitMenu_Platform(void)
{
    static const PlatformMenu calls = { KitMenu_Start, KitMenu_Stop, KitMenu_Event, KitMenu_Frame, KitMenu_IsOpen,
        KitMenu_Hide, KitMenu_Show, KitMenu_DrawOver };
    return &calls;
}

void KitMenu_Start(SDL_Window *window, void (*deliver)(const SDL_Event *event))
{
    if (menu || window == nullptr || Harness_Active()) return;
    deliverToGame = deliver;
    try
    {
        menu = std::make_unique<rekit::sdl::GameMenu>(hooks(window), SetupHost_Theme());
        Upgrades_SetSetupMenu(open_setup_menu);
    }
    catch (const std::exception &e)
    {
        std::fprintf(stderr, "setup menu: none (%s)\n", e.what());
        menu.reset();
    }
}

void KitMenu_Stop(void)
{
    Upgrades_SetSetupMenu(nullptr);
    menu.reset();
    if (overlay) SDL_DestroyTexture(overlay);
    overlay = nullptr;
    pixels.clear();
    showing = false;
}

int KitMenu_Event(const SDL_Event *event)
{
    if (!menu) return 0;
    (void) menu->event(*event);
    return 1;
}

int KitMenu_Frame(int now)
{
    if (!menu) return 0;
    const uint64_t at = SDL_GetTicks();
    if (!now && at - lastFrameMS < 16) return 0;
    lastFrameMS = at;
    const uint64_t before = pictureSerial;
    const bool wasShowing = showing;
    drawnThisFrame = false;
    menu->frame();
    showing = drawnThisFrame;
    return showing != wasShowing || (showing && pictureSerial != before);
}

int KitMenu_IsOpen(void)
{
    return menu && menu->is_open();
}

void KitMenu_Show(void)
{
    if (menu) menu->show();
}

void KitMenu_Hide(void)
{
    if (menu && menu->is_open()) menu->hide();
}

void KitMenu_DrawOver(SDL_Renderer *renderer)
{
    if (!showing || pixels.empty() || renderer == nullptr) return;
    if (overlay)
    {
        float w = 0, h = 0;
        SDL_GetTextureSize(overlay, &w, &h);
        if (int(w) != pictureW || int(h) != pictureH || SDL_GetRendererFromTexture(overlay) != renderer)
        {
            SDL_DestroyTexture(overlay);
            overlay = nullptr;
        }
    }
    if (!overlay)
    {
        overlay = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, pictureW, pictureH);
        if (!overlay) return;
        SDL_SetTextureBlendMode(overlay, SDL_BLENDMODE_BLEND);
        uploadedSerial = pictureSerial - 1;
    }
    if (uploadedSerial != pictureSerial)
    {
        SDL_UpdateTexture(overlay, nullptr, pixels.data(), pictureW * 4);
        uploadedSerial = pictureSerial;
    }
    // Over the whole window at its pixel size, not the game's 320x200 letterbox.
    int lw = 0, lh = 0;
    SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
    SDL_GetRenderLogicalPresentation(renderer, &lw, &lh, &mode);
    SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    SDL_RenderTexture(renderer, overlay, nullptr, nullptr);
    SDL_SetRenderLogicalPresentation(renderer, lw, lh, mode);
}
