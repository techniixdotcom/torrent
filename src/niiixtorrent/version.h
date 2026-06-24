#pragma once

#include <winver.h>

#define STRINGIZE2(s) #s
#define STRINGIZE(s) STRINGIZE2(s)

#define VER_FILE_DESCRIPTION_STR    "NiiX Torrent"
#define VER_FILE_VERSION            NIIX_VERSION_MAJOR, NIIX_VERSION_MINOR, NIIX_VERSION_PATCH
#define VER_FILE_VERSION_STR        STRINGIZE(NIIX_VERSION_MAJOR)        \
                                    "." STRINGIZE(NIIX_VERSION_MINOR)    \
                                    "." STRINGIZE(NIIX_VERSION_PATCH) \
                                    " (" STRINGIZE(NIIX_GIT_COMMITISH) ")" \

#define VER_PRODUCTNAME_STR         "NiiX Torrent"
#define VER_PRODUCT_VERSION         VER_FILE_VERSION
#define VER_PRODUCT_VERSION_STR     VER_FILE_VERSION_STR
#define VER_ORIGINAL_FILENAME_STR   "NiiXTorrent.exe"
#define VER_INTERNAL_NAME_STR       VER_ORIGINAL_FILENAME_STR
#define VER_COMPANYNAME_STR         "techniix"
#define VER_COPYRIGHT_STR           "Copyright (C) 2026 techniix"

#ifdef _DEBUG
  #define VER_VER_DEBUG             VS_FF_DEBUG
#else
  #define VER_VER_DEBUG             0
#endif

#define VER_FILEOS                  0x00040004
#define VER_FILEFLAGS               VER_VER_DEBUG
#define VER_FILETYPE                0x00000001
