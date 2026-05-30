# File Manager

A cross-platform desktop file manager built with **C++ and Qt**, currently fully functional on Linux with Windows and macOS support planned.

---

## Features

- Browse files and folders with grid (icon) or list view
- Navigate via clickable path bar or type a path directly
- Home and Up buttons for quick navigation
- Create folders, rename, delete files and folders
- Copy, cut and paste files
- Mark files as favourites (★)
- Right-click context menu with all file actions
- **Open With** — launch any installed app directly from the file manager
- File properties dialog (name, path, size, last modified)
- Dark and light theme toggle
- Status bar showing item count, folder count, favourites, and current selection

---

## Project Structure

```
file-manager/
├── src/
│   ├── main.cpp
│   ├── ui/
│   │   ├── MainWindow.cpp       # Qt UI, all views and user interactions
│   │   └── MainWindow.hpp
│   ├── core/
│   │   ├── FileSystem.cpp       # File operations (copy, cut, paste, rename, delete, favourites)
│   │   └── FileSystem.hpp
│   │   ├── AppManager.cpp       # Installed app discovery and executable resolution
│   │   └── AppManager.hpp
│   └── config/
│       └── AppConfig.hpp        # Window size constants and defaults
└── resources/
    └── icons/
```

---

## How It Works

### File Browsing
`FileSystem` tracks the current directory and exposes `getAllFileInCurrentDir()` which returns a list of `FileInfo` structs — name, path, type, size, whether it's a directory, and whether it's marked as a favourite. `MainWindow` renders these into either a `QListWidget` in icon mode (grid) or a `QTreeWidget` (list view).

### App Discovery — AppManager
On Linux, installed apps are discovered by scanning `.desktop` files per the [freedesktop.org spec](https://specifications.freedesktop.org/desktop-entry-spec/latest/). These are plain key=value text files that describe an application — its name, executable, icon, and supported MIME types.

Directories scanned:

| Location | Contents |
|---|---|
| `/usr/share/applications` | System-wide apps (apt/deb installs) |
| `/usr/local/share/applications` | Locally compiled apps |
| `~/.local/share/applications` | User-installed apps |
| `/var/lib/flatpak/exports/share/applications` | System Flatpak apps |
| `~/.local/share/flatpak/exports/share/applications` | User Flatpak apps |
| `/var/lib/snapd/desktop/applications` | Snap apps |
| `~/snap` | User-level Snap apps |

Each `.desktop` file is parsed to extract the app name, executable, icon, and MIME types. The `Exec=` line is cleaned of runtime placeholders (`%f`, `%u`, `%F`, `%U` etc.) and the executable is resolved to its full path.

#### Executable Resolution
Two-stage resolution to handle all install types:

1. **`resolveExecutable`** — checks if the path is absolute and exists, otherwise walks the `PATH` environment variable
2. **`resolveSnapExecutable`** — fallback for Snap apps; checks `/snap/bin/<name>` first (the wrapper script), then walks `/snap/<name>/current/usr/bin/` for each installed snap

Apps are deduplicated by matching both `execPath` and `name` so that entries like `code.desktop` and `code-url-handler.desktop` (same binary, different app names) are kept as separate entries while true duplicates from overlapping scan directories are removed.

### Open With
Right-clicking any file, folder, or empty space shows an "Open With" submenu. It currently calls `getAllApps()` to show every discovered app. The submenu is implemented as a `QWidgetAction` containing a `QScrollArea` so it scrolls cleanly when there are many apps, instead of overflowing off screen.

Clicking an app calls `FileSystem::openWith(appPath, filePath)` which launches the app with the selected file as an argument.

---

## Building

### Prerequisites
- CMake 3.20+
- Qt 6 
- A C++17 compiler

### Linux
```bash
cmake --build build
cd build
./File_Manager
```

---

## Platform Support

| Platform | Status |
|---|---|
| Linux | ✅ Fully working |
| Windows | 🚧 Not implemented |
| macOS | 🚧 Not implemented |

### Windows 
App discovery needs to read from the Windows registry:
- `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths`
- `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths`

Or enumerate Start Menu `.lnk` shortcuts. File association for "Open With" would use registry extension mappings.

### macOS 
App discovery would scan `/Applications` recursively and read each app's `Info.plist` for `CFBundleExecutable`, `CFBundleName`, and `CFBundleDocumentTypes`. File association would use `LSCopyApplicationURLsForURL` from the Launch Services API.

---

## Known Issues and Limitations

### Open With — not optimal
The current approach has a few problems worth knowing about:

- **Shows all apps, not filtered ones.** `getAllApps()` is called instead of `getAppForFile()` because MIME filtering was too aggressive and hid valid apps. The right fix is smarter MIME matching — e.g. falling back to the file extension, or checking parent MIME types (e.g. `text/*` matching `text/plain`).

- **MIME detection spawns a process.** `getMimeTypeForFile` shells out to the `file` command via `popen()`. This works but is slow if called repeatedly. A better approach would be to use `libmagic` directly (the C library behind the `file` command) or Qt's `QMimeDatabase` which does this in-process.

- **No default app concept.** There is no way to mark or remember a preferred app for a file type. The list is always the full set of installed apps in alphabetical order.

- **App icons not shown.** The `AppInfo` struct stores the icon name from the `.desktop` file but the UI currently shows only the app name as text in the Open With menu. Resolving icon names to actual image files (via the XDG icon theme spec) is not yet implemented.

- **No `xdg-open` fallback.** On Linux the standard way to open a file with its default app is `xdg-open`. This is not currently used, so files without a manually chosen app have no default action on double-click for non-directory entries.

---
## ScreenShot
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/5742c86c-f6ff-4c9b-8df8-d61796a2e80e" />
<img width="1920" height="1080" alt="image" src="https://github.com/user-attachments/assets/d7818e31-ce1d-41a8-8dd6-789678415ba5" />



## Contributing

The codebase is structured to make platform implementations self-contained — each platform's code lives inside `#if defined(__linux__)` / `#elif defined(_WIN32)` / `#elif defined(__APPLE__)` blocks. Adding Windows or macOS support means filling in those TODO blocks in `AppManager.cpp` without touching the Linux code.
