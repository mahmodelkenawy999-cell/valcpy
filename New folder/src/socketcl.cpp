#include <stdafx.hpp>
#include "socketcl.hpp"

socket_client_t::~socket_client_t( )
{
    disconnect( );
    if ( m_wsa_initialized )
    {
        WSACleanup( );
        m_wsa_initialized = false;
    }
}

bool socket_client_t::connect( const std::string& host, int port )
{
    if ( !m_wsa_initialized )
    {
        WSADATA wsa_data;
        if ( WSAStartup( MAKEWORD( 2, 2 ), &wsa_data ) != 0 )
            return false;
        m_wsa_initialized = true;
    }

    m_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( m_socket == INVALID_SOCKET )
        return false;

    DWORD timeout = 2000;
    setsockopt( m_socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast< const char* >( &timeout ), sizeof( timeout ) );
    setsockopt( m_socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast< const char* >( &timeout ), sizeof( timeout ) );

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons( static_cast< u_short >( port ) );

    if ( host == "localhost" )
        addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
    else
        inet_pton( AF_INET, host.c_str( ), &addr.sin_addr );

    if ( ::connect( m_socket, reinterpret_cast< sockaddr* >( &addr ), sizeof( addr ) ) == SOCKET_ERROR )
    {
        closesocket( m_socket );
        m_socket = INVALID_SOCKET;
        return false;
    }

    timeout = 100;
    setsockopt( m_socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast< const char* >( &timeout ), sizeof( timeout ) );

    return true;
}

void socket_client_t::disconnect( )
{
    if ( m_socket != INVALID_SOCKET )
    {
        closesocket( m_socket );
        m_socket = INVALID_SOCKET;
    }
}

bool socket_client_t::send_click( )
{
    if ( m_socket == INVALID_SOCKET )
        return false;

    const char* data = "click";
    int result = send( m_socket, data, static_cast< int >( strlen( data ) ), 0 );
    return result != SOCKET_ERROR;
}
