#pragma once

class socket_client_t
{
public:
    ~socket_client_t( );

    bool connect( const std::string& host = "localhost", int port = 65433 );
    void disconnect( );
    bool is_connected( ) const { return m_socket != INVALID_SOCKET; }

    bool send_click( );

private:
    SOCKET m_socket = INVALID_SOCKET;
    bool m_wsa_initialized = false;
};
