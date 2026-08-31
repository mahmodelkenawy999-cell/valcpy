#include <stdafx.hpp>
#include "cfg.hpp"
#include "cap.hpp"
#include "dtc.hpp"
#include "socketcl.hpp"
#include "socketsrv.hpp"
#include "gui.hpp"
#include "../utils/input/input.hpp"
#include "../utils/keys.hpp"

constexpr int SWITCH_KEY = VK_F1;
constexpr int EXIT_KEY = VK_F2;
constexpr auto CONFIG_PATH = "config.ini";
constexpr auto SOCKET_HOST = "localhost";
constexpr int SOCKET_PORT = 65433;

struct mode_profile_t
{
    const char* label;
    int color;
    double fire_rate;
};

const mode_profile_t MODE_PROFILES[ 3 ] = {
    { "LOW   ", 2, 0.4 },
    { "MEDIUM", 6, 0.3 },
    { "HIGH  ", 4, 0.001 }
};

class simple_console_t
{
public:
    bool initialize( const char* title )
    {
        m_handle = GetStdHandle( STD_OUTPUT_HANDLE );
        SetConsoleTitleA( title );

        DWORD mode;
        if ( GetConsoleMode( m_handle, &mode ) )
            SetConsoleMode( m_handle, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING );

        CONSOLE_CURSOR_INFO cursor{ 1, FALSE };
        SetConsoleCursorInfo( m_handle, &cursor );

        clear( );
        return m_handle != INVALID_HANDLE_VALUE;
    }

    void clear( )
    {
        system( "cls" );
    }

    void set_color( int color )
    {
        SetConsoleTextAttribute( m_handle, color );
    }

    void print( const char* msg )
    {
        set_color( 7 );
        printf( "  %s\n", msg );
    }

    void success( const char* msg )
    {
        set_color( 10 );
        printf( "  [+] %s\n", msg );
        set_color( 7 );
    }

    void warn( const char* msg )
    {
        set_color( 14 );
        printf( "  [!] %s\n", msg );
        set_color( 7 );
    }

    void error( const char* msg )
    {
        set_color( 12 );
        printf( "  [X] %s\n", msg );
        set_color( 7 );
        system( "pause" );
        ExitProcess( 1 );
    }

private:
    HANDLE m_handle = INVALID_HANDLE_VALUE;
};

// ==================== CLIENT MODE ====================
class trigger_bot_t
{
public:
    bool initialize( )
    {
        if ( !m_console.initialize( "valcore - [Client]" ) )
            return false;

        if ( !m_config.load( CONFIG_PATH ) )
        {
            m_console.warn( "config.ini not found, using defaults" );
            m_config.save( CONFIG_PATH );
        }

        if ( !m_capture.initialize( m_config.scan_area, m_config.scan_area ) )
            m_console.error( "failed to initialize screen capture" );

        m_console.print( "connecting to host..." );
        while ( !m_socket.connect( SOCKET_HOST, SOCKET_PORT ) )
        {
            m_console.warn( "waiting for host..." );
            std::this_thread::sleep_for( 1s );
        }
        m_console.success( "connected to host" );

        m_running = true;
        return true;
    }

    void run( )
    {
        display_status( );

        auto last_fire = std::chrono::steady_clock::now( );
        bool switch_pressed = false;

        while ( m_running )
        {
            auto now = std::chrono::steady_clock::now( );

            if ( GetAsyncKeyState( EXIT_KEY ) & 0x8000 )
            {
                m_console.print( "exiting..." );
                break;
            }

            if ( GetAsyncKeyState( SWITCH_KEY ) & 0x8000 )
            {
                if ( !switch_pressed )
                {
                    switch_pressed = true;
                    m_mode = ( m_mode + 1 ) % 3;
                    display_status( );
                    std::this_thread::sleep_for( 200ms );
                }
            }
            else
            {
                switch_pressed = false;
            }

            // Check for Alt key (both left and right)
            bool alt_pressed = ( GetAsyncKeyState( VK_LMENU ) & 0x8000 ) || ( GetAsyncKeyState( VK_RMENU ) & 0x8000 );
            
            if ( alt_pressed )
            {
                auto elapsed = std::chrono::duration< double >( now - last_fire ).count( );
                if ( elapsed >= MODE_PROFILES[ m_mode ].fire_rate )
                {
                    if ( process_frame( ) )
                        last_fire = now;
                }
            }

            std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
        }
    }

private:
    bool process_frame( )
    {
        pixel_data_t frame = m_capture.capture( );
        if ( !frame.data )
            return false;

        if ( m_detector.detect_red( frame, m_config ) )
        {
            if ( !m_socket.send_click( ) )
            {
                m_console.warn( "connection lost, reconnecting..." );
                m_socket.disconnect( );
                while ( !m_socket.connect( SOCKET_HOST, SOCKET_PORT ) )
                    std::this_thread::sleep_for( 500ms );
                m_console.success( "reconnected" );
            }
            return true;
        }
        return false;
    }

    void display_status( )
    {
        const auto& profile = MODE_PROFILES[ m_mode ];
        int rpm = static_cast< int >( 60.0 / profile.fire_rate );
        std::string key_name = utilities::get_key_name( m_config.hold_key );

        m_console.clear( );
        m_console.set_color( 3 );
        printf( "\n     - valcore -\n\n" );
        m_console.set_color( 7 );
        m_console.set_color( profile.color );
        printf( "  Mode: %s\n", profile.label );
        m_console.set_color( 7 );
        printf( "  Rate: %d RPM (%.3fs)\n", rpm, profile.fire_rate );
        printf( "  FOV : %dpx\n", m_config.scan_area );
        printf( "  Sens: %d\n", m_config.color_sens );
        printf( "  Key : %s\n", key_name.c_str( ) );
        printf( "  Stat: Connected\n" );
        printf( "\n  F1 - Switch Mode\n" );
        printf( "  F2 - Exit\n" );
    }

    simple_console_t m_console;
    config_t m_config;
    screen_capture_t m_capture;
    detector_t m_detector;
    socket_client_t m_socket;
    int m_mode = 0;
    std::atomic< bool > m_running{ false };
};

// ==================== HOST MODE ====================
class host_mode_t
{
public:
    bool initialize( )
    {
        if ( !m_console.initialize( "valcore [Host]" ) )
            return false;

        if ( !m_server.start( ) )
            m_console.error( "failed to start server" );

        if ( !m_input.initialize( ) )
            m_console.error( "failed to initialize input" );

        m_console.success( "host started on localhost:65433" );
        m_console.print( "waiting for client connections..." );
        return true;
    }

    void run( )
    {
        std::thread server_thread( [ this ]( ) { m_server.run_loop( ); } );

        while ( true )
        {
            if ( GetAsyncKeyState( VK_F2 ) & 0x8000 )
                break;
            std::this_thread::sleep_for( 100ms );
        }

        m_console.print( "shutting down..." );
        m_server.stop( );
        server_thread.join( );
    }

private:
    simple_console_t m_console;
    socket_server_t m_server;
    input m_input;
};

// ==================== MAIN ====================
int WINAPI WinMain( _In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int )
{
    gui_t gui;
    if ( !gui.initialize( ) )
    {
        MessageBoxA( nullptr, "Failed to initialize GUI", "valcore Error", MB_OK | MB_ICONERROR );
        return 1;
    }

    MSG msg;
    while ( gui.is_running( ) )
    {
        while ( PeekMessage( &msg, nullptr, 0U, 0U, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }

        if ( !gui.is_running( ) )
            break;

        gui.render( );
    }

    gui.shutdown( );
    return 0;
}
