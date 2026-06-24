# NiiX Torrent — Revival & Rebrand Notes

This is a modernized, rebranded continuation of the discontinued PicoTorrent
project.

## Rebrand (PicoTorrent -> NiiX Torrent)

- Product name, window titles, message boxes, About dialog, taskbar tooltip.
- Version resource (`version.h`): product name, description, original filename
  (`NiiXTorrent.exe`), copyright.
- BitTorrent identity: user agent `NiiXTorrent/x.y.z`; peer-ID fingerprint
  prefix changed from `-PI-` to `-NX-`.
- HTTP/GeoIP user agents, crash-report release tag, IPC service name, autorun
  registry value, AppData folder, SQLite DB and log filenames, `HKCU\Software`
  key.
- MSI installer (WiX): product/feature/shortcut names, file associations and
  ProgIds, firewall rule, registry capabilities — all reissued with **fresh
  GUIDs** so NiiX Torrent installs as a distinct product.
- All 37 language files and the documentation.

## Dark theme only

- The theme selector has been removed from Preferences.
- `Configuration::IsDarkMode()` always returns `true`.
- Native Windows dark mode is forced at startup via
  `MSWEnableDarkMode(DarkMode_Always)`.
- Default `theme_id` migration value set to `"dark"`.

## Dependency / toolchain modernization

- `vcpkg.json`: schema added, OpenSSL made explicit; versions resolve from the
  `vendor/vcpkg` checkout (pull it to get the newest packages).
- wxWidgets pinned to the 3.3 stable line (first release with native dark mode).
- CMake minimum raised to 3.25; ANTLR4 runtime and utfcpp bumped; deprecated
  `git://` protocol replaced with `https://`.
- CI workflow updated to `actions/checkout@v4` and `actions/upload-artifact@v4`.
- Cake tool bumped to 4.0.0.

## Reliability decisions (documented on purpose)

- **C++ standard kept at C++17** (the standard the codebase was validated
  against) rather than forcing C++20, to avoid introducing compile breaks.
- **`/WX` (warnings-as-errors) relaxed to `/W4`** so deprecation warnings from
  newer dependency versions do not fail the build. `/guard:cf` (control-flow
  guard) is retained.
- The optional .NET installer bootstrapper keeps its internal
  `NiiXTorrentBootstrapper` assembly/namespace for build stability; only its
  user-facing strings are rebranded.
- The original backend endpoints (update check, GeoIP database) pointed at the
  discontinued project's server. They now use NiiX placeholders and are inert
  until you host your own.

## Assets

Icon and image assets in `res/` (`app.ico`, `file.ico`, etc.) are retained from
the original project. Replace them with custom NiiX artwork when ready.
