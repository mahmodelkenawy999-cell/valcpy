#pragma once
#include "../utils/input/input.hpp"
#include <chrono>

class socket_server_t
{
public:
    ~socket_server_t( );

    bool start( const std::string& host = "localhost", int port = 65433 );
    void stop( );
    bool is_running( ) const { return m_running; }

    void run_loop( );

private:
    void handle_client( SOCKET client );
    void display_info( );

    SOCKET m_listen_socket = INVALID_SOCKET;
    std::atomic< bool > m_running{ false };
    bool m_wsa_initialized = false;

    input m_input;
    double m_min_click_interval = 0.01;
    std::chrono::steady_clock::time_point m_last_click;
};
