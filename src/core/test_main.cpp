#include <iostream>
#include <cassert>
#include <filesystem>
#include <fstream>
#include "Filesystem.hpp"
#include "AppManager.hpp"

// ─────────────────────────────────────────────
//  Simple test helpers
// ─────────────────────────────────────────────

static int passed = 0;
static int failed = 0;

// Checks a condition, prints PASS or FAIL with the test name
void check(bool condition, const std::string& testName) {
    if (condition) {
        std::cout << "  [PASS] " << testName << "\n";
        passed++;
    } else {
        std::cout << "  [FAIL] " << testName << "\n";
        failed++;
    }
}

// Prints a section header
void section(const std::string& name) {
    std::cout << "\n── " << name << " ──\n";
}

// ─────────────────────────────────────────────
//  Test setup helpers
// ─────────────────────────────────────────────

namespace fs = std::filesystem;

// Creates a temporary directory with some test files inside
// Returns the path to the temp dir
fs::path setupTempDir() {
    fs::path tmp = fs::temp_directory_path() / "fm_test";
    fs::create_directories(tmp);

    // Create a few test files
    std::ofstream(tmp / "hello.txt")  << "hello world";
    std::ofstream(tmp / "data.json")  << "{}";
    std::ofstream(tmp / "image.png")  << "fake png content";

    // Create a subdirectory
    fs::create_directories(tmp / "subdir");

    return tmp;
}

// Removes the temp directory after tests
void cleanupTempDir(const fs::path& tmp) {
    std::error_code ec;
    fs::remove_all(tmp, ec);  // ec so it doesn't throw if already gone
}

// ─────────────────────────────────────────────
//  FileSystem Tests
// ─────────────────────────────────────────────

void testFileSystem() {
    std::cout << "\n╔══════════════════════════════╗\n";
    std::cout <<   "║   FileSystem Tests           ║\n";
    std::cout <<   "╚══════════════════════════════╝\n";

    fs::path tmp = setupTempDir();
    FileSystem fm;
    fm.updatePath(tmp);

    // ── Navigation ───────────────────────────
    section("Navigation");

    check(fm.getCurrentFilePath() == tmp,
        "getCurrentFilePath() returns correct path");

    fm.toParent();
    check(fm.getCurrentFilePath() == tmp.parent_path(),
        "toParent() moves up one level");

    fm.updatePath(tmp);
    check(fm.getCurrentFilePath() == tmp,
        "updatePath() sets path correctly");

    // ── File Listing ─────────────────────────
    section("File listing");

    auto files = fm.getAllFileInCurrentDir();
    check(!files.empty(),
        "getAllFileInCurrentDir() returns files");

    // Check that our created files appear in the list
    bool foundTxt = false, foundDir = false;
    for (const auto& f : files) {
        if (f.name == "hello.txt") foundTxt = true;
        if (f.name == "subdir")    foundDir = true;
    }
    check(foundTxt, "hello.txt is listed");
    check(foundDir, "subdir is listed");

    // Check isDir is set correctly
    for (const auto& f : files) {
        if (f.name == "subdir") {
            check(f.isDir,  "subdir.isDir == true");
            check(f.size == 0, "subdir.size == 0");
        }
        if (f.name == "hello.txt") {
            check(!f.isDir, "hello.txt.isDir == false");
            check(f.extension == ".txt", "hello.txt.extension == .txt");
        }
    }

    // ── Create Directory ─────────────────────
    section("Create directory");

    bool created = fm.createDirectory("newFolder");
    check(created, "createDirectory() returns true");
    check(fs::exists(tmp / "newFolder"), "newFolder actually exists on disk");

    // ── Rename ───────────────────────────────
    section("Rename");

    bool renamed = fm.renamePath("newFolder", "renamedFolder");
    check(renamed, "renamePath() returns true");
    check(!fs::exists(tmp / "newFolder"),    "old name no longer exists");
    check(fs::exists(tmp / "renamedFolder"), "new name exists on disk");

    // ── Delete ───────────────────────────────
    section("Delete");

    bool deleted = fm.deletePath("renamedFolder");
    check(deleted, "deletePath() returns true");
    check(!fs::exists(tmp / "renamedFolder"), "deleted folder no longer exists");

    // ── Copy & Paste ─────────────────────────
    section("Copy & Paste");

    fm.copy("hello.txt");
    fm.updatePath(tmp / "subdir");

    bool pasted = fm.paste();
    check(pasted, "paste() returns true after copy");
    check(fs::exists(tmp / "subdir" / "hello.txt"), "file was copied to subdir");

    // Pasting again to same location should fail (file already exists)
    bool pasteAgain = fm.paste();
    check(!pasteAgain, "paste() returns false when destination already exists");

    fm.updatePath(tmp);

    // ── Cut & Paste ──────────────────────────
    section("Cut & Paste");

    // Create a file to cut
    std::ofstream(tmp / "cut_me.txt") << "cut content";
    fm.cut("cut_me.txt");
    fm.updatePath(tmp / "subdir");

    bool cutPasted = fm.paste();
    check(cutPasted, "paste() returns true after cut");
    check(!fs::exists(tmp / "cut_me.txt"),           "cut file no longer in source");
    check(fs::exists(tmp / "subdir" / "cut_me.txt"), "cut file exists in destination");

    fm.updatePath(tmp);

    // ── Properties ───────────────────────────
    section("Properties");

    auto props = fm.getProperties(tmp / "hello.txt");
    check(props.name == "hello.txt",   "getProperties().name is correct");
    check(props.path == tmp / "hello.txt", "getProperties().path is correct");
    check(props.parentPath == tmp,     "getProperties().parentPath is correct");

    // ── Favourites ───────────────────────────
    section("Favourites");

    // Directories cannot be favourited
    bool dirFav = fm.ToggleFav(tmp / "subdir");
    check(!dirFav, "ToggleFav() returns false for directories");

    // Note: ToggleFav saves to FAV_FILE which is "" (empty) in your current
    // config — so we just check it doesn't crash. Set FAV_FILE to a real
    // path to test full save/load behaviour.
    std::cout << "  [SKIP] ToggleFav file save/load — FAV_FILE not configured\n";

    cleanupTempDir(tmp);
    std::cout << "\n  Temp directory cleaned up.\n";
}

// ─────────────────────────────────────────────
//  AppManager Tests
// ─────────────────────────────────────────────

void testAppManager() {
    std::cout << "\n╔══════════════════════════════╗\n";
    std::cout <<   "║   AppManager Tests           ║\n";
    std::cout <<   "╚══════════════════════════════╝\n";

    AppManager am;

    // ── getAllApps ───────────────────────────
    section("getAllApps()");

    auto allApps = am.getAllApps();
    check(!allApps.empty(), "getAllApps() finds at least one app");

    // Every app must have a name and a valid execPath
    bool allHaveNames = true;
    bool allHavePaths = true;
    for (const auto& app : allApps) {
        if (app.name.empty())    allHaveNames = false;
        if (app.execPath.empty()) allHavePaths = false;
    }
    check(allHaveNames, "every app has a name");
    check(allHavePaths, "every app has an execPath");

    // Print a sample so you can visually verify
    std::cout << "\n  Sample of found apps (first 5):\n";
    int shown = 0;
    for (const auto& app : allApps) {
        if (shown++ >= 5) break;
        std::cout << "    • " << app.name
                  << " → " << app.execPath.string() << "\n";
        if (!app.mimeTypes.empty()) {
            std::cout << "      MIME: " << app.mimeTypes[0];
            if (app.mimeTypes.size() > 1)
                std::cout << " (+" << app.mimeTypes.size()-1 << " more)";
            std::cout << "\n";
        }
    }

    // ── refresh() ───────────────────────────
    section("refresh()");

    size_t countBefore = allApps.size();
    am.refresh();
    auto afterRefresh = am.getAllApps();
    check(afterRefresh.size() == countBefore,
        "refresh() reloads same number of apps");

    // ── getAppsForFile() ─────────────────────
    section("getAppsForFile()");

    // Create a temp text file to test filtering
    fs::path tmpFile = fs::temp_directory_path() / "fm_test_mime.txt";
    std::ofstream(tmpFile) << "test content";

    auto appsForTxt = am.getAppForFile(tmpFile);
    check(!appsForTxt.empty(), "getAppsForFile() finds apps for a .txt file");

    std::cout << "\n  Apps that can open a .txt file (first 3):\n";
    int txtShown = 0;
    for (const auto& app : appsForTxt) {
        if (txtShown++ >= 3) break;
        std::cout << "    • " << app.name << "\n";
    }

    // Test with a non-existent file — should return all apps as fallback
    fs::path fakePath = "/tmp/nonexistent_file_xyz.mp4";
    auto appsForFake = am.getAppForFile(fakePath);
    // It either returns all apps (fallback) or empty — either is acceptable
    std::cout << "  [INFO] getAppsForFile() on non-existent file returned "
              << appsForFake.size() << " apps (fallback behaviour)\n";

    // Cleanup
    std::error_code ec;
    fs::remove(tmpFile, ec);
}

// ─────────────────────────────────────────────
//  Main
// ─────────────────────────────────────────────

int main() {
    std::cout << "╔══════════════════════════════════╗\n";
    std::cout << "║   File Manager — Test Suite      ║\n";
    std::cout << "╚══════════════════════════════════╝\n";

    testFileSystem();
    testAppManager();

    // ── Summary ──────────────────────────────
    std::cout << "\n╔══════════════════════════════════╗\n";
    std::cout << "║   Results                        ║\n";
    std::cout << "╠══════════════════════════════════╣\n";
    std::cout << "║  Passed : " << passed
              << std::string(23 - std::to_string(passed).size(), ' ') << "║\n";
    std::cout << "║  Failed : " << failed
              << std::string(23 - std::to_string(failed).size(), ' ') << "║\n";
    std::cout << "╚══════════════════════════════════╝\n";

    return failed > 0 ? 1 : 0;  // exit code 1 if any test failed
}