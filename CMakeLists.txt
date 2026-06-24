cmake_minimum_required(VERSION 3.25 FATAL_ERROR)
cmake_policy(SET CMP0092 NEW) # don't add /W3 as default
cmake_policy(SET CMP0115 OLD) # Source file extensions must be explicit.

# set(VCPKG_OVERLAY_PORTS    ${CMAKE_CURRENT_SOURCE_DIR}/vendor/vcpkg-overlays/ports)
set(VCPKG_OVERLAY_TRIPLETS ${CMAKE_CURRENT_SOURCE_DIR}/vendor/vcpkg-overlays/triplets)
set(CMAKE_TOOLCHAIN_FILE   ${CMAKE_CURRENT_SOURCE_DIR}/vendor/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "Vcpkg toolchain file")

project("NiiXTorrent")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------- antlr4 options
option(WITH_STATIC_CRT "" Off)
# -----------------------

# --------- wx options
set(wxBUILD_SHARED        OFF   CACHE BOOL   "" FORCE)
set(wxBUILD_COMPATIBILITY "3.2" CACHE STRING "" FORCE)
set(wxUSE_EXCEPTIONS      OFF   CACHE BOOL   "" FORCE)
# --------------------

# vendor libs
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vendor/sentry-crashpad/CMakeLists.txt")
    set(WITH_CRASHPAD ON)
    add_subdirectory(vendor/sentry-crashpad)
else()
    set(WITH_CRASHPAD OFF)
    message(STATUS "NiiX: vendor/sentry-crashpad not present - building WITHOUT crash reporting")
endif()
add_subdirectory(vendor/wx)

# niix components
add_subdirectory(src/pql)

# vcpkg packages
find_package(Boost                      REQUIRED COMPONENTS log)
find_package(fmt                 CONFIG REQUIRED)
find_package(LibtorrentRasterbar CONFIG REQUIRED)
find_package(nlohmann_json       CONFIG REQUIRED)
find_package(unofficial-sqlite3  CONFIG REQUIRED)

# Generate buildinfo file
configure_file("${CMAKE_SOURCE_DIR}/src/niixtorrent/buildinfo.cpp.in" "${CMAKE_SOURCE_DIR}/src/niixtorrent/buildinfo.cpp" @ONLY)

# Debug flags
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi")

# Release flags
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/DEBUG /INCREMENTAL:NO /MAP /OPT:REF /OPT:ICF")
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "/DEBUG /INCREMENTAL:NO /MAP /OPT:REF /OPT:ICF")

add_executable(
    NiiXTorrent-coredb
    src/l10n/main
    lang/ca-ES.json
    lang/de-DE.json
    lang/el-GR.json
    lang/en-US.json
    lang/es-ES.json
    lang/fi-FI.json
    lang/fr-FR.json
    lang/hu-HU.json
    lang/hy-AM.json
    lang/id-ID.json
    lang/it-IT.json
    lang/ka-GE.json
    lang/ko-KR.json
    lang/lt-LT.json
    lang/lv-LV.json
    lang/nl-NL.json
    lang/pl-PL.json
    lang/pt-BR.json
    lang/ro-RO.json
    lang/ru-RU.json
    lang/sv-SE.json
    lang/tr-TR.json
    lang/uk-UA.json
    lang/zh-CN.json
)

target_link_libraries(
    NiiXTorrent-coredb
    PRIVATE
    nlohmann_json::nlohmann_json
    unofficial::sqlite3::sqlite3
)

add_executable(
    NiiXTorrent
    WIN32

    src/niixtorrent/application
    src/niixtorrent/buildinfo
    src/niixtorrent/crashpadinitializer
    src/niixtorrent/main
    src/niixtorrent/persistencemanager
    src/niixtorrent/resources.rc

    # API
    src/niixtorrent/api/libniix

    # BitTorrent
    src/niixtorrent/bittorrent/session
    src/niixtorrent/bittorrent/torrenthandle

    # Core
    src/niixtorrent/core/configuration
    src/niixtorrent/core/database
    src/niixtorrent/core/environment
    src/niixtorrent/core/utils

    # Http
    src/niixtorrent/http/httpclient

    # IPC
    src/niixtorrent/ipc/applicationoptionsconnection
    src/niixtorrent/ipc/server

    # Dialogs
    src/niixtorrent/ui/dialogs/aboutdialog
    src/niixtorrent/ui/dialogs/addmagnetlinkdialog
    src/niixtorrent/ui/dialogs/addtrackerdialog
    src/niixtorrent/ui/dialogs/addtorrentdialog
    src/niixtorrent/ui/dialogs/createtorrentdialog
    src/niixtorrent/ui/dialogs/listeninterfacedialog
    src/niixtorrent/ui/dialogs/preferencesadvancedpage
    src/niixtorrent/ui/dialogs/preferencesconnectionpage
    src/niixtorrent/ui/dialogs/preferencesdialog
    src/niixtorrent/ui/dialogs/preferencesdownloadspage
    src/niixtorrent/ui/dialogs/preferencesgeneralpage
    src/niixtorrent/ui/dialogs/preferenceslabelspage
    src/niixtorrent/ui/dialogs/preferencesproxypage
    src/niixtorrent/ui/dialogs/textoutputdialog

    # Filters
    src/niixtorrent/ui/filters/pqltorrentfilter

    # Models
    src/niixtorrent/ui/models/filestoragemodel
    src/niixtorrent/ui/models/peerlistmodel
    src/niixtorrent/ui/models/torrentlistmodel
    src/niixtorrent/ui/models/trackerlistmodel

    # Persistence
    src/niixtorrent/ui/persistence/persistenttorrentlistview

    # UI
    src/niixtorrent/ui/console
    src/niixtorrent/ui/mainframe
    src/niixtorrent/ui/statusbar
    src/niixtorrent/ui/taskbaricon
    src/niixtorrent/ui/torrentcontextmenu
    src/niixtorrent/ui/torrentdetailsfilespanel
    src/niixtorrent/ui/torrentdetailsoverviewpanel
    src/niixtorrent/ui/torrentdetailspeerspanel
    src/niixtorrent/ui/torrentdetailstrackerspanel
    src/niixtorrent/ui/torrentdetailsview
    src/niixtorrent/ui/torrentfilelistview
    src/niixtorrent/ui/torrentlistview
    src/niixtorrent/ui/translator

    # Widgets
    src/niixtorrent/ui/widgets/pieceprogressbar

    # Win32 specific stuff
    src/niixtorrent/ui/win32/openfiledialog
)

target_compile_definitions(
    NiiXTorrent
    PRIVATE
    -D_UNICODE
    -D_WIN32
    -D_WIN32_WINNT=0x0A00
    -DNOMINMAX
    -DPICOJSON_USE_INT64
    -DUNICODE
    -DWIN32
    -DWIN32_LEAN_AND_MEAN

    -DLIBNIIX_API_EXPORT

    -DNIIX_GIT_COMMITISH=${GITVERSION_VAR_SHORTSHA}
    -DNIIX_VERSION_MAJOR=${GITVERSION_VAR_VERSION_MAJOR}
    -DNIIX_VERSION_MINOR=${GITVERSION_VAR_VERSION_MINOR}
    -DNIIX_VERSION_PATCH=${GITVERSION_VAR_VERSION_PATCH}

    -DNIIX_WX_GIT_COMMITISH=${NIIX_WX_GIT_COMMITISH}
)

target_compile_options(NiiXTorrent PRIVATE /guard:cf /W4)

target_include_directories(
    NiiXTorrent
    PRIVATE

    include
)

target_link_libraries(
    NiiXTorrent

    PRIVATE

    # boost
    Boost::boost
    Boost::log

    # wxWidgets
    wxcore wxbase wxpropgrid

    # Windows
    Comctl32
    crypt32
    iphlpapi
    legacy_stdio_definitions
    propsys
    shlwapi
    wininet
    winhttp

    # fmt
    fmt::fmt

    # nlohmann-json
    nlohmann_json::nlohmann_json

    # Rasterbar-libtorrent
    LibtorrentRasterbar::torrent-rasterbar

    # PQL
    NiiXTorrentPQL

    unofficial::sqlite3::sqlite3
)

set_property(TARGET NiiXTorrent PROPERTY ENABLE_EXPORTS 1)

# Plugins
add_library(
    Plugin_Updater
    SHARED
    src/plugins/updater/updater
)

target_compile_definitions(Plugin_Updater PRIVATE -D_WIN32 -DUNICODE -DWIN32_LEAN_AND_MEAN)
target_include_directories(Plugin_Updater PRIVATE include)
target_link_libraries(Plugin_Updater PRIVATE NiiXTorrent Comctl32)

# Crashpad (optional crash reporting)
if(WITH_CRASHPAD)
    target_compile_definitions(NiiXTorrent PRIVATE NIIX_WITH_CRASHPAD)
    target_link_libraries(NiiXTorrent PRIVATE crashpad_client crashpad_util mini_chromium)
    add_custom_command(
        TARGET NiiXTorrent POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:crashpad_handler>
            $<TARGET_FILE_DIR:NiiXTorrent>
    )
endif()

# Generate coredb
add_custom_command(
    TARGET NiiXTorrent POST_BUILD
    COMMAND NiiXTorrent-coredb "${CMAKE_SOURCE_DIR}/lang" $<TARGET_FILE_DIR:NiiXTorrent>
)
