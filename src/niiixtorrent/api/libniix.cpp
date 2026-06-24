#include "libniix_impl.hpp"

#include <Windows.h>

#include <vector>

#include "../bittorrent/torrenthandle.hpp"
#include "../bittorrent/torrentstatus.hpp"
#include "../buildinfo.hpp"
#include "../core/configuration.hpp"
#include "../core/environment.hpp"
#include "../core/utils.hpp"
#include "../http/httpclient.hpp"
#include "../ui/mainframe.hpp"
#include "../ui/translator.hpp"

using pt::API::IPlugin;

struct libniix_http_response_t
{
    std::string data;
};

class Plugin : public IPlugin
{
public:
    Plugin(HMODULE hModule, pt::Core::Environment* env, pt::Core::Configuration* cfg)
        : m_hModule(hModule),
        m_env(env),
        m_cfg(cfg)
    {
        libniix_init_t init = reinterpret_cast<libniix_init_t>(
            GetProcAddress(
                m_hModule,
                LIBNIIX_TOSTRING(LIBNIIX_INIT)));

        init(LIBNIIX_API_VERSION,
            reinterpret_cast<libniix_plugin_t*>(this));
    }

    virtual ~Plugin()
    {
        EmitEvent(libniix_event_shutdown, nullptr);
    }

    pt::Core::Configuration* Configuration() { return m_cfg; }
    pt::Core::Environment* Environment() { return m_env; }

    void AddHook(libniix_hook_callback_t callback, libniix_param_t* param)
    {
        m_hooks.push_back({ callback, param });
    }

    void EmitEvent(libniix_event_t event, void* param)
    {
        for (auto const& cb : m_hooks)
        {
            cb.callback(event, reinterpret_cast<libniix_param_t*>(param), cb.param);
        }
    }

private:
    struct Hook
    {
        libniix_hook_callback_t callback;
        libniix_param_t* param;
    };

    pt::Core::Environment* m_env;
    pt::Core::Configuration* m_cfg;
    HMODULE m_hModule;
    std::vector<Hook> m_hooks;
};

IPlugin::~IPlugin()
{
}

IPlugin* IPlugin::Load(fs::path const& p, pt::Core::Environment* env, pt::Core::Configuration* cfg)
{
    return new Plugin(
        LoadLibrary(p.wstring().c_str()),
        env,
        cfg);
}

libniix_result_t libniix_config_get(libniix_plugin_t* plugin, libniix_config_t** config)
{
    auto p = reinterpret_cast<Plugin*>(plugin);
    *config = reinterpret_cast<libniix_config_t*>(p->Configuration());
    return libniix_ok;
}

libniix_result_t libniix_config_bool_get(libniix_config_t* cfg, const char* key, bool* result)
{
    auto config = reinterpret_cast<pt::Core::Configuration*>(cfg);
    *result = config->Get<bool>(key).value();
    return libniix_ok;
}

libniix_result_t libniix_config_string_get(libniix_config_t* cfg, const char* key, char* result, size_t* len)
{
    auto config = reinterpret_cast<pt::Core::Configuration*>(cfg);
    auto res = config->Get<std::string>(key).value_or("");
    strncpy(result, res.c_str(), *len);
    *len = res.size();
    return libniix_ok;
}

libniix_result_t libniix_config_string_set(libniix_config_t* config, const char* key, const char* value, size_t len)
{
    std::string val = len == (size_t)-1
        ? std::string(value)
        : std::string(value, len);

    reinterpret_cast<pt::Core::Configuration*>(config)->Set(key, val);

    return libniix_ok;
}

libniix_result_t libniix_http_get(const char* url, libniix_http_callback_t callback, libniix_param_t* user)
{
    auto client = std::make_shared<pt::Http::HttpClient>();
    client->Get(url, [client, callback, user](int statusCode, std::string const& data)
        {
            libniix_http_response_t response;
            response.data = data;
            callback(&response, (libniix_http_status_t)statusCode, user);
        });

    return libniix_ok;
}

libniix_result_t libniix_http_response_body(libniix_http_response_t* response, char* body, size_t len)
{
    strncpy(body, response->data.c_str(), len);
    return libniix_ok;
}

libniix_result_t libniix_http_response_body_len(libniix_http_response_t* response, size_t* len)
{
    *len = response->data.size();
    return libniix_ok;
}

libniix_result_t libniix_i18n(const char* key, wchar_t* target, size_t* len)
{
    wxString translation = pt::UI::Translator::GetInstance().Translate(key);
    size_t ll = len == nullptr
        ? wcslen(target)
        : *len;

    if (len != nullptr)
    {
        *len = translation.size();
    }

    if (translation.size() > ll || target == nullptr)
    {
        return libniix_insufficient_buffer;
    }

    wcsncpy(target, translation.wc_str(), ll);

    return libniix_ok;
}

libniix_result_t libniix_mainwnd_native_handle(libniix_mainwnd_t* wnd, void** handle)
{
    *handle = reinterpret_cast<pt::UI::MainFrame*>(wnd)->GetHWND();
    return libniix_ok;
}

libniix_result_t libniix_menu_get(libniix_mainwnd_t* wnd, libniix_menu_id_t id, libniix_menu_t** menu)
{
    auto mf = reinterpret_cast<pt::UI::MainFrame*>(wnd);
    auto menuBar = mf->GetMenuBar();

    switch (id)
    {
    case libniix_menu_help:
        *menu = reinterpret_cast<libniix_menu_t*>(menuBar->GetMenu(2));
        return libniix_ok;
    }

    return libniix_err;
}

libniix_result_t libniix_menu_insert_item(libniix_menu_t* menu, uint32_t pos, const wchar_t* label, size_t len, libniix_menuitem_callback_t cb, libniix_param_t* user, libniix_menuitem_t** item)
{
    auto m = reinterpret_cast<wxMenu*>(menu);
    auto itm = m->Insert(pos, 9999, wxString(label, len));
    m->Bind(
        wxEVT_MENU,
        [menu, item, cb, user](wxCommandEvent&)
        {
            cb(nullptr, user);
        },
        9999);

    if (item != nullptr)
    {
        *item = reinterpret_cast<libniix_menuitem_t*>(itm);
    }

    return libniix_ok;
}

libniix_result_t libniix_menu_insert_separator(libniix_menu_t* menu, uint32_t pos)
{
    auto m = reinterpret_cast<wxMenu*>(menu);
    m->InsertSeparator(pos);
    return libniix_ok;
}

libniix_result_t libniix_register_hook(libniix_plugin_t* plugin, libniix_hook_callback_t cb, libniix_param_t* user)
{
    reinterpret_cast<Plugin*>(plugin)->AddHook(cb, user);
    return libniix_ok;
}

libniix_result_t libniix_string_towide(const char* input, wchar_t* output, size_t len)
{
    auto out = pt::Utils::toStdWString(input);
    wcsncpy(output, out.c_str(), len);
    return libniix_ok;
}

libniix_result_t libniix_torrent_stats_get(libniix_torrent_t* torrent, libniix_torrent_stats_t* stats)
{
    auto t = reinterpret_cast<pt::BitTorrent::TorrentHandle*>(torrent);
    auto s = t->Status();

    stats->download_payload_rate = s.downloadPayloadRate;
    stats->upload_payload_rate = s.uploadPayloadRate;

    return libniix_ok;
}

const char* libniix_version()
{
    return pt::BuildInfo::version();
}
