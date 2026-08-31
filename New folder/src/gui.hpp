#pragma once

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include "socketsrv.hpp"
#include "socketcl.hpp"
#include "cfg.hpp"
#include "cap.hpp"
#include "dtc.hpp"

class gui_t
{
public:
    bool initialize( );
    void shutdown( );
    void render( );
    void add_log( const std::string& message, int color = 7 );
    bool is_running( ) const { return m_running; }

private:
    void render_host_tab( );
    void render_client_tab( );
    void render_console( );
    void render_status_badge( bool active );
    void render_section_header( const char* label );
    void host_thread_func( );
    void client_thread_func( );
    void push_style( );

    bool                  m_running           = false;
    bool                  m_menu_open         = true;
    HWND                  m_hwnd              = nullptr;

    bool                  m_host_initialized  = false;
    socket_server_t       m_server;
    std::thread           m_host_thread;
    std::atomic<bool>     m_host_running{ false };

    bool                  m_client_initialized = false;
    config_t              m_config;
    screen_capture_t      m_capture;
    detector_t            m_detector;
    socket_client_t       m_socket;
    std::thread           m_client_thread;
    std::atomic<bool>     m_client_running{ false };
    int                   m_delay_mode        = 1;
    bool                  m_waiting_for_key   = false;

    struct log_entry_t { std::string message; int color; };
    std::vector<log_entry_t> m_console_logs;
    bool                     m_auto_scroll = true;
};