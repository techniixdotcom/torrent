#include <libniix.h>

#include <string>
#include <Windows.h>
#include <CommCtrl.h>
#include <shellapi.h>

#pragma warning(push)
#pragma warning(disable: 4200)
#pragma warning(disable: 4996)
#include "sajson.h"
#include "semver.hpp"
#pragma warning(pop)

#define DEFAULT_I18N_BUFFER_SIZE 256

struct updater_request_data_t
{
    bool force;
    libniix_config_t* config;
    libniix_mainwnd_t* wnd;
};

void show_available_update(libniix_mainwnd_t* wnd, libniix_config_t* config, const char* version, const char* url)
{
    HWND hWnd = nullptr;
    libniix_mainwnd_native_handle(wnd, reinterpret_cast<void**>(&hWnd));

    // get wide version
    wchar_t versionw[100];
    libniix_string_towide(version, versionw, 100);

    wchar_t content[DEFAULT_I18N_BUFFER_SIZE];
    size_t content_len = DEFAULT_I18N_BUFFER_SIZE;
    libniix_i18n("new_version_available", content, &content_len);

    wchar_t main[DEFAULT_I18N_BUFFER_SIZE];
    wchar_t main_format[DEFAULT_I18N_BUFFER_SIZE];
    size_t main_len = DEFAULT_I18N_BUFFER_SIZE;
    libniix_i18n("niixtorrent_v_available", main, &main_len);
    swprintf(main_format, DEFAULT_I18N_BUFFER_SIZE, main, versionw);

    wchar_t verification[DEFAULT_I18N_BUFFER_SIZE];
    size_t verification_len = DEFAULT_I18N_BUFFER_SIZE;
    libniix_i18n("ignore_update", verification, &verification_len);

    wchar_t show[DEFAULT_I18N_BUFFER_SIZE];
    size_t show_len = DEFAULT_I18N_BUFFER_SIZE;
    libniix_i18n("show_on_github", show, &show_len);

    const TASKDIALOG_BUTTON pButtons[] =
    {
        { 1000, show },
    };

    TASKDIALOGCONFIG tdf = { sizeof(TASKDIALOGCONFIG) };
    tdf.cButtons = ARRAYSIZE(pButtons);
    tdf.dwCommonButtons = TDCBF_CLOSE_BUTTON;
    tdf.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW | TDF_USE_COMMAND_LINKS;
    tdf.hwndParent = hWnd;
    tdf.pButtons = pButtons;
    tdf.pszMainIcon = TD_INFORMATION_ICON;
    tdf.pszMainInstruction = main_format;
    tdf.pszVerificationText = verification;
    tdf.pszWindowTitle = TEXT("NiiX Torrent");

    int pnButton = -1;
    int pnRadioButton = -1;
    BOOL pfVerificationFlagChecked = FALSE;

    TaskDialogIndirect(&tdf, &pnButton, &pnRadioButton, &pfVerificationFlagChecked);

    if (pnButton == 1000)
    {
        wchar_t urlw[255];
        libniix_string_towide(url, urlw, 255);

        SHELLEXECUTEINFO sei;
        ZeroMemory(&sei, sizeof(SHELLEXECUTEINFO));

        sei.cbSize = sizeof(SHELLEXECUTEINFO);
        sei.lpFile = urlw;
        sei.lpVerb = TEXT("open");
        sei.nShow = SW_SHOWNORMAL;
        sei.fMask = SEE_MASK_FLAG_NO_UI;

        ::ShellExecuteEx(&sei);
    }

    if (pfVerificationFlagChecked)
    {
        libniix_config_string_set(
            config,
            "update_checks.ignored_version",
            version,
            (size_t)-1);
    }
}

void show_no_update(libniix_mainwnd_t* wnd)
{
    HWND hWnd = nullptr;
    libniix_mainwnd_native_handle(wnd, reinterpret_cast<void**>(&hWnd));

    wchar_t main[DEFAULT_I18N_BUFFER_SIZE];
    size_t main_len = DEFAULT_I18N_BUFFER_SIZE;
    libniix_i18n("no_update_available", main, &main_len);

    TASKDIALOGCONFIG tdf = { sizeof(TASKDIALOGCONFIG) };
    tdf.dwCommonButtons = TDCBF_OK_BUTTON;
    tdf.dwFlags = TDF_POSITION_RELATIVE_TO_WINDOW;
    tdf.hwndParent = hWnd;
    tdf.pszMainIcon = TD_INFORMATION_ICON;
    tdf.pszMainInstruction = main;
    tdf.pszWindowTitle = L"NiiX Torrent";

    TaskDialogIndirect(&tdf, nullptr, nullptr, nullptr);
}

libniix_result_t parse_response(
    libniix_http_response_t* response,
    libniix_http_status_t status,
    libniix_param_t* user)
{
    updater_request_data_t* data = reinterpret_cast<updater_request_data_t*>(user);

    switch (status)
    {
    case libniix_http_ok:
    {
        size_t len;
        libniix_http_response_body_len(response, &len);

        if (len > 0)
        {
            std::string body(len, '\0');
            libniix_http_response_body(response, body.data(), body.size());

            const sajson::document& doc = sajson::parse(
                sajson::dynamic_allocation(),
                sajson::mutable_string_view(body.size(), body.data()));

            if (!doc.is_valid())
            {
                // TODO: log
                break;
            }

            const sajson::value& root = doc.get_root();

            std::string version = root.get_value_of_key(sajson::literal("version")).as_string();

            char ignoredVersion[100];
            size_t ignoredVersionLen = 100;

            libniix_config_string_get(
                data->config,
                "update_checks.ignored_version",
                ignoredVersion,
                &ignoredVersionLen);

            if (version == std::string(ignoredVersion, ignoredVersionLen) && !data->force)
            {
                break;
            }

            semver::version parsedVersion(version);
            semver::version currentVersion(libniix_version());

            if (parsedVersion > currentVersion)
            {
                std::string url = root.get_value_of_key(sajson::literal("url")).as_string();

                show_available_update(
                    data->wnd,
                    data->config,
                    version.c_str(),
                    url.c_str());
            }
            else if (data->force)
            {
                show_no_update(data->wnd);
            }
        }
        break;
    }
    }

    return libniix_ok;
}

void make_request(libniix_config_t* config, libniix_mainwnd_t* wnd, bool force)
{
    updater_request_data_t* data = new updater_request_data_t();
    data->config = config;
    data->force = force;
    data->wnd = wnd;

    char url[255];
    size_t url_len = 255;

    libniix_config_string_get(config, "update_checks.url", url, &url_len);
    libniix_http_get(url, parse_response, reinterpret_cast<libniix_param_t*>(data));
}

libniix_result_t check(libniix_menuitem_t* item, libniix_param_t* param)
{
    updater_request_data_t* data = reinterpret_cast<updater_request_data_t*>(param);
    make_request(data->config, data->wnd, data->force);
    return libniix_ok;
}

libniix_result_t on_events(
    libniix_event_t event,
    libniix_param_t* param,
    libniix_param_t* user)
{
    libniix_plugin_t* plugin = reinterpret_cast<libniix_plugin_t*>(user);

    switch (event)
    {
    case libniix_event_mainwnd_created:
    {
        libniix_config_t* config = nullptr;
        libniix_config_get(plugin, &config);

        bool enabled = false;
        libniix_config_bool_get(config, "update_checks.enabled", &enabled);

        if (enabled)
        {
            make_request(
                config,
                reinterpret_cast<libniix_mainwnd_t*>(param),
                false);
        }

        // Insert item in about menu to force a check for update
        libniix_menu_t* help;
        libniix_menu_get(reinterpret_cast<libniix_mainwnd_t*>(param), libniix_menu_help, &help);

        wchar_t check_for_update[DEFAULT_I18N_BUFFER_SIZE];
        size_t check_for_update_len = DEFAULT_I18N_BUFFER_SIZE;
        libniix_i18n("amp_check_for_update", check_for_update, &check_for_update_len);

        updater_request_data_t* data = new updater_request_data_t();
        data->config = config;
        data->force = true;
        data->wnd = reinterpret_cast<libniix_mainwnd_t*>(param);

        libniix_menu_insert_item(
            help,
            0,
            check_for_update,
            check_for_update_len,
            check,
            reinterpret_cast<libniix_param_t*>(data),
            nullptr);

        libniix_menu_insert_separator(
            help,
            1);

        break;
    }
    }

    return libniix_ok;
}

libniix_result_t init_updater(int version, libniix_plugin_t* plugin)
{
    if (version != LIBNIIX_API_VERSION)
    {
        return libniix_version_mismatch;
    }

    return libniix_register_hook(plugin, on_events, reinterpret_cast<libniix_param_t*>(plugin));
}

LIBNIIX_DEFINE_PLUGIN(
    "updater",
    "1.0",
    init_updater);
