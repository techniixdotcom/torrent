#ifndef LIBNIIX_H
#define LIBNIIX_H

#include <stdint.h>

#define LIBNIIX_API_VERSION 1
#define LIBNIIX_INIT libniix_init
#define LIBNIIX_STRINGIFY(x) #x
#define LIBNIIX_TOSTRING(x) LIBNIIX_STRINGIFY(x)

#define LIBNIIX_DEFINE_PLUGIN(name,version,init_func) \
    extern "C" __declspec(dllexport) libniix_result_t LIBNIIX_INIT(int v, libniix_plugin_t* a) \
    { \
        return init_func(v,a); \
    }

#if defined(LIBNIIX_API_EXPORT)
#define LIBNIIX_API_FUNCTION \
    extern "C" __declspec(dllexport)
#else
#define LIBNIIX_API_FUNCTION \
    extern "C" __declspec(dllimport)
#endif

typedef struct libniix_config_t libniix_config_t;
typedef struct libniix_http_response_t libniix_http_response_t;
typedef struct libniix_mainwnd_t libniix_mainwnd_t;
typedef struct libniix_menu_t libniix_menu_t;
typedef struct libniix_menuitem_t libniix_menuitem_t;
typedef struct libniix_param_t libniix_param_t;
typedef struct libniix_plugin_t libniix_plugin_t;
typedef struct libniix_torrent_t libniix_torrent_t;

enum libniix_event_t
{
    libniix_event_mainwnd_created,
    libniix_event_shutdown
};

enum libniix_http_status_t
{
    libniix_http_ok = 200,
    libniix_http_not_found = 404,
    libniix_http_internal_server_error = 500
};

enum libniix_menu_id_t
{
    libniix_menu_file,
    libniix_menu_view,
    libniix_menu_help
};

enum libniix_result_t
{
    libniix_ok,
    libniix_err,
    libniix_version_mismatch,
    libniix_insufficient_buffer
};

typedef struct libniix_torrent_stats_t
{
    int32_t download_payload_rate;
    int32_t upload_payload_rate;
} libniix_torrent_stats_t;

typedef libniix_result_t(*libniix_http_callback_t)(libniix_http_response_t*, libniix_http_status_t, libniix_param_t*);
typedef libniix_result_t(*libniix_init_t)(int, libniix_plugin_t*);
typedef libniix_result_t(*libniix_hook_callback_t)(libniix_event_t, libniix_param_t*, libniix_param_t*);
typedef libniix_result_t(*libniix_menuitem_callback_t)(libniix_menuitem_t*, libniix_param_t*);

/*
Config.
*/
LIBNIIX_API_FUNCTION libniix_result_t libniix_config_get(libniix_plugin_t* plugin, libniix_config_t** config);
LIBNIIX_API_FUNCTION libniix_result_t libniix_config_bool_get(libniix_config_t* config, const char* key, bool* result);
LIBNIIX_API_FUNCTION libniix_result_t libniix_config_string_get(libniix_config_t* config, const char* key, char* result, size_t* len);
LIBNIIX_API_FUNCTION libniix_result_t libniix_config_string_set(libniix_config_t* config, const char* key, const char* value, size_t len);

/*
HTTP.
*/
LIBNIIX_API_FUNCTION libniix_result_t libniix_http_get(const char* url, libniix_http_callback_t cb, libniix_param_t * user);
LIBNIIX_API_FUNCTION libniix_result_t libniix_http_response_body(libniix_http_response_t* response, char* body, size_t len);
LIBNIIX_API_FUNCTION libniix_result_t libniix_http_response_body_len(libniix_http_response_t* response, size_t* len);

LIBNIIX_API_FUNCTION libniix_result_t libniix_i18n(const char* key, wchar_t* target, size_t* len);

LIBNIIX_API_FUNCTION libniix_result_t libniix_mainwnd_native_handle(libniix_mainwnd_t* wnd, void** handle);

/*
Menu.
*/
LIBNIIX_API_FUNCTION libniix_result_t libniix_menu_get(libniix_mainwnd_t* wnd, libniix_menu_id_t id, libniix_menu_t** menu);
LIBNIIX_API_FUNCTION libniix_result_t libniix_menu_insert_item(libniix_menu_t* menu, uint32_t pos, const wchar_t* label, size_t len, libniix_menuitem_callback_t cb, libniix_param_t* param, libniix_menuitem_t** item);
LIBNIIX_API_FUNCTION libniix_result_t libniix_menu_insert_separator(libniix_menu_t* menu, uint32_t pos);

/*
String functions
*/
LIBNIIX_API_FUNCTION libniix_result_t libniix_string_towide(const char* input, wchar_t* output, size_t len);

LIBNIIX_API_FUNCTION libniix_result_t libniix_register_hook(libniix_plugin_t* app, libniix_hook_callback_t cb, libniix_param_t* user);
LIBNIIX_API_FUNCTION libniix_result_t libniix_torrent_stats_get(libniix_torrent_t* torrent, libniix_torrent_stats_t* stats);

LIBNIIX_API_FUNCTION const char* libniix_version();
#endif
