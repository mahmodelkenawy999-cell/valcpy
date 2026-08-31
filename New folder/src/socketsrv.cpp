#include <stdafx.hpp>
#include "socketsrv.hpp"

static std::string get_timestamp( )
{
    auto now = std::chrono::system_clock::now( );
    auto time = std::chrono::system_clock::to_time_t( now );
    std::stringstream ss;
    ss << std::put_time( std::localtime( &time ), "%H:%M:%S" );
    return ss.str( );
}

socket_server_t::~socket_server_t( )
{
    stop( );
}

bool socket_server_t::start( const std::string& host, int port )
{
    WSADATA wsa_data;
    if ( WSAStartup( MAKEWORD( 2, 2 ), &wsa_data ) != 0 )
        return false;
    m_wsa_initialized = true;

    m_listen_socket = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( m_listen_socket == INVALID_SOCKET )
        return false;

    // Allow address reuse
    int reuse = 1;
    setsockopt( m_listen_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast< const char* >( &reuse ), sizeof( reuse ) );

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons( static_cast< u_short >( port ) );

    // Handle "localhost" vs IP address
    if ( host == "localhost" )
        addr.sin_addr.s_addr = inet_addr( "127.0.0.1" );
    else
        inet_pton( AF_INET, host.c_str( ), &addr.sin_addr );

    if ( bind( m_listen_socket, reinterpret_cast< sockaddr* >( &addr ), sizeof( addr ) ) == SOCKET_ERROR )
    {
        closesocket( m_listen_socket );
        m_listen_socket = INVALID_SOCKET;
        return false;
    }

    if ( listen( m_listen_socket, 1 ) == SOCKET_ERROR )
    {
        closesocket( m_listen_socket );
        m_listen_socket = INVALID_SOCKET;
        return false;
    }

    // Set timeout for accept
    DWORD timeout = 1000;  // 1 second
    setsockopt( m_listen_socket, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast< const char* >( &timeout ), sizeof( timeout ) );

    // Initialize input system
    if ( !m_input.initialize( ) )
    {
        closesocket( m_listen_socket );
        m_listen_socket = INVALID_SOCKET;
        return false;
    }

    m_running = true;
    return true;
}

void socket_server_t::stop( )
{
    m_running = false;
    if ( m_listen_socket != INVALID_SOCKET )
    {
        closesocket( m_listen_socket );
        m_listen_socket = INVALID_SOCKET;
    }
    if ( m_wsa_initialized )
    {
        WSACleanup( );
        m_wsa_initialized = false;
    }
}

void socket_server_t::display_info( )
{
    system( "cls" );
    HANDLE handle = GetStdHandle( STD_OUTPUT_HANDLE );
    SetConsoleTextAttribute( handle, 11 );
    printf( "\n     - valcore [Host] -\n\n" );
    SetConsoleTextAttribute( handle, 7 );

    printf( "  Host   : localhost\n" );
    printf( "  Port   : 65433\n" );
    printf( "  Stat   : Connected\n\n" );
}

void socket_server_t::run_loop( )
{
    while ( m_running )
    {
        sockaddr_in client_addr{};
        int addr_len = sizeof( client_addr );

        SOCKET client = accept( m_listen_socket, reinterpret_cast< sockaddr* >( &client_addr ), &addr_len );
        if ( client == INVALID_SOCKET )
        {
            int error = WSAGetLastError( );
            if ( error == WSAEWOULDBLOCK || error == WSAETIMEDOUT )
                continue;
            break;
        }

        display_info( );

        DWORD timeout = 100;
        setsockopt( client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast< const char* >( &timeout ), sizeof( timeout ) );

        handle_client( client );
        closesocket( client );
        printf( "  [!] client disconnected\n" );
    }
}

void socket_server_t::handle_client( SOCKET client )
{
    char buffer[ 64 ];

    while ( m_running )
    {
        int received = recv( client, buffer, sizeof( buffer ) - 1, 0 );
        if ( received <= 0 )
        {
            int error = WSAGetLastError( );
            if ( error == WSAEWOULDBLOCK || error == WSAETIMEDOUT )
                continue;
            break;
        }

        buffer[ received ] = '\0';

        if ( strcmp( buffer, "click" ) == 0 )
        {
            auto now = std::chrono::steady_clock::now( );
            auto elapsed = std::chrono::duration< double >( now - m_last_click ).count( );

            if ( elapsed >= m_min_click_interval )
            {
                POINT pt;
                GetCursorPos( &pt );

                m_input.inject_mouse( pt.x, pt.y, input::left_down );
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
                m_input.inject_mouse( pt.x, pt.y, input::left_up );

                m_last_click = now;
            }
        }
    }
}
