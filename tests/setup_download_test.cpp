// The original files from the launcher's catalog (the kit's Game::original_catalogs): the recipe names the catalog,
// its key and the game's entry (chr), and a fresh setup that finds no copy offers the download and gets it. The
// catalog here is a local one, signed by the kit's test key, its source a zip built in the work folder (file:// stands
// in for https://, REKIT_SETUP_ALLOW_FILE): nothing is fetched from the network. With a copy of the game
// (REVENGE_GAME) the zip holds its files and the install is ready; without one it holds a stand-in, offered and
// fetched but not installed. Prints names and counts only; nothing of the game is kept.
//
//   setup_download_test WORK_DIR

#include "SDL/setup_host.h"
#include "Setup/Setup.h"
#include "test_keys.h"

#include <rekit/hash.h>
#include <rekit/sdl/pickers.h>
#include <rekit/sdl/setup_screen.h>
#include <rekit/status.h>

#include <SDL3/SDL.h>

#if REKIT_HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;
namespace setup = chr::setup;

namespace
{

int failures = 0;

void check(bool ok, const std::string &what)
{
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
    if (!ok) ++failures;
}

// The recipe's catalog: the launcher's two addresses, its key, the game's entry, this build's tag.
void recipe(const fs::path &work)
{
    const rekit::sdl::Game g = SetupHost_KitGame((work / "user").string());
    check(g.original_catalogs.size() == 2 &&
            g.original_catalogs[0] == "https://reverse-engineered.com/launcher/catalog.json" &&
            g.original_catalogs[1] == "https://raw.githubusercontent.com/telltaleatheist/mw3-re/main/catalog/games.json",
        "the recipe asks the launcher's catalog, then its copy on GitHub");
    check(g.catalog_keys.size() == 1 && g.catalog_keys[0] == setup::kCatalogKey && g.catalog_keys[0].size() == 64,
        "the recipe checks the catalog's signature against the launcher's key");
    check(g.original_key == "chr", "the recipe's entry is the game's (chr)");
    check(g.version == setup::build_tag(), "the recipe's version is the build's release tag (\"" + g.version + "\")");
}

#if REKIT_HAVE_LIBARCHIVE
std::string read(const fs::path &p)
{
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

void write(const fs::path &p, const std::string &text)
{
    fs::create_directories(p.parent_path());
    std::ofstream(p, std::ios::binary) << text;
}

// A zip of `files` (name in the archive, its bytes).
bool make_zip(const fs::path &out, const std::vector<std::pair<std::string, std::string>> &files)
{
    archive *a = archive_write_new();
    archive_write_set_format_zip(a);
    if (archive_write_open_filename(a, out.string().c_str()) != ARCHIVE_OK)
    {
        archive_write_free(a);
        return false;
    }
    for (const auto &[name, data] : files)
    {
        archive_entry *e = archive_entry_new();
        archive_entry_set_pathname(e, name.c_str());
        archive_entry_set_size(e, static_cast<la_int64_t>(data.size()));
        archive_entry_set_filetype(e, AE_IFREG);
        archive_entry_set_perm(e, 0644);
        archive_write_header(a, e);
        archive_write_data(a, data.data(), data.size());
        archive_entry_free(e);
    }
    archive_write_close(a);
    archive_write_free(a);
    return true;
}

// A file:// address for `p`, percent-encoded, so a checkout's path may hold a space.
std::string file_url(const fs::path &p)
{
    static const char *hex = "0123456789ABCDEF";
    std::string url = "file://";
    for (const char ch : p.generic_string())
    {
        const auto c = static_cast<unsigned char>(ch);
        if (std::isalnum(c) || std::strchr("/-._~", c))
        {
            url += ch;
        }
        else
        {
            url += '%';
            url += hex[c >> 4];
            url += hex[c & 15];
        }
    }
    return url;
}

bool has(const std::vector<std::string> &log, const std::string &line)
{
    for (const std::string &l : log)
        if (l == line) return true;
    return false;
}

bool starts(const std::vector<std::string> &log, const std::string &head)
{
    for (const std::string &l : log)
        if (l.rfind(head, 0) == 0) return true;
    return false;
}

// A fresh setup that finds no copy: the download offered, pressed, fetched, and installed when it is the game.
void download(const fs::path &work, const char *game)
{
    std::vector<std::pair<std::string, std::string>> files;
    if (game)
    {
        for (const fs::directory_entry &e : fs::directory_iterator(game))
            if (e.is_regular_file()) files.push_back({ "revenge/" + e.path().filename().string(), read(e.path()) });
    }
    else
    {
        files = { { "revenge/REVENGE.EXE", "a stand-in" } };
    }
    const fs::path served = work / "served", zip = served / "Revenge_1990.zip";
    fs::create_directories(served);
    check(make_zip(zip, files), "a zip built in the work folder (" + std::to_string(files.size()) + " files)");
    const std::string bytes = read(zip);
    const std::string sum = rekit::sha256_hex(reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size());
    const std::string source = "{\"url\": \"" + file_url(zip) + "\", \"sha256\": \"" + sum +
        "\", \"size\": " + std::to_string(bytes.size()) +
        ", \"name\": \"Revenge_1990.zip\", \"note\": \"a test\"}";
    const std::string text =
        "{\"revision\": 9, \"games\": [{\"key\": \"chr\", \"name\": \"Revenge Rebuild\", \"original\": [" + source + "]}]}";
    const fs::path cat = work / "catalog.json";
    write(cat, text);
    write(cat.string() + ".sig", test_keys::sign(text, 1));

    const fs::path user = work / "user";
    rekit::sdl::Game g = SetupHost_KitGame(user.string());
    g.original_catalogs = { file_url(cat) };
    g.catalog_keys = { test_keys::public_key(1) };
    g.search = []
    { return std::string(); };  // nothing on this computer: the test never looks at the real one

    std::vector<std::string> log;
    rekit::sdl::SetupOptions o;
    o.user_folder = user.string(), o.default_out = setup::default_install(user.string()), o.hidden = true;
    o.temp_dir = (work / "temp").string();
    fs::create_directories(o.temp_dir);
    o.log = [&log](const std::string &l)
    { log.push_back(l); };
    // The kit reads its hooks through SDL's environment, which a setenv after SDL starts would not reach.
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "REKIT_SETUP_ALLOW_FILE", "1", true);
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "REKIT_SETUP_DOWNLOAD", "1", true);
    SDL_SetEnvironmentVariable(SDL_GetEnvironment(), "REKIT_SETUP_FRAMES", "60", true);
    rekit::sdl::unattended() = []
    { return true; };
    int pickers = 0;
    rekit::sdl::picker_opener() = [&pickers](const rekit::sdl::PickerAsk &)
    { ++pickers; };
    const rekit::sdl::SetupResult r = rekit::sdl::run_setup(g, rekit::sdl::Theme{}, o);

    check(has(log, "start: no copy found on this computer"), "a fresh setup searched this computer first");
    check(has(log, "screen find: DOWNLOAD THE GAME FILES, PICK MY OWN COPY, QUIT"), "no copy found: the download offered");
    check(has(log, "work: GETTING THE ORIGINAL GAME FILES") && has(log, "original: downloaded Revenge_1990.zip"),
        "the download pressed and fetched from the catalog's source");
    check(pickers == 0, "no picker opened");
    if (game)
    {
        check(r.exit == rekit::kSetupReady &&
                setup::check_install(setup::default_install(user.string())).state == rekit::Status::kReady,
            "the download installed, ready");
    }
    else
    {
        check(r.exit != rekit::kSetupReady && !starts(log, "work: INSTALLING"), "a stand-in download is not installed");
        std::printf("skipped: the install (no REVENGE_GAME)\n");
    }
    if (failures)
        for (const std::string &l : log) std::printf("    | %s\n", l.c_str());
}
#endif

}  // namespace

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::fprintf(stderr, "usage: setup_download_test WORK_DIR\n");
        return 2;
    }
    const fs::path work = fs::absolute(argv[1]).lexically_normal();  // a catalog's addresses hold no ".."
    std::error_code ec;
    fs::remove_all(work, ec);
    fs::create_directories(work);
    recipe(work);
#if REKIT_HAVE_LIBARCHIVE
    const char *game = std::getenv("REVENGE_GAME");
    download(work, game && *game ? game : nullptr);
#else
    std::printf("skipped: the download (this build has no libarchive)\n");
#endif
    fs::remove_all(work, ec);  // nothing of the game's kept
    std::printf("%d failed\n", failures);
    return failures ? 1 : 0;
}
