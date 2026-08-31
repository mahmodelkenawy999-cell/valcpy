#include <stdafx.hpp>
#include "gui.hpp"
#include "../utils/imgui/imgui.h"
#include "../utils/imgui/imgui_impl_win32.h"
#include "../utils/imgui/imgui_impl_dx11.h"
#include "../utils/keys.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dwmapi.h>

static ID3D11Device*            g_pd3dDevice          = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext    = nullptr;
static IDXGISwapChain*          g_pSwapChain           = nullptr;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static gui_t*                   g_gui_instance         = nullptr;

namespace pal
{
    constexpr ImVec4 bg_window  { 0.07f, 0.07f, 0.09f, 0.97f };
    constexpr ImVec4 bg_panel   { 0.10f, 0.10f, 0.13f, 1.00f };
    constexpr ImVec4 bg_input   { 0.13f, 0.13f, 0.17f, 1.00f };
    constexpr ImVec4 accent     { 0.25f, 0.55f, 1.00f, 1.00f };
    constexpr ImVec4 accent_dim { 0.18f, 0.38f, 0.72f, 1.00f };
    constexpr ImVec4 accent_hov { 0.35f, 0.65f, 1.00f, 1.00f };
    constexpr ImVec4 green      { 0.20f, 0.85f, 0.45f, 1.00f };
    constexpr ImVec4 red        { 1.00f, 0.30f, 0.30f, 1.00f };
    constexpr ImVec4 yellow     { 1.00f, 0.80f, 0.10f, 1.00f };
    constexpr ImVec4 text_dim   { 0.50f, 0.52f, 0.58f, 1.00f };
    constexpr ImVec4 text_label { 0.70f, 0.72f, 0.80f, 1.00f };
    constexpr ImVec4 text_white { 0.92f, 0.93f, 0.96f, 1.00f };
}

LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND, UINT, WPARAM, LPARAM );
    if ( ImGui_ImplWin32_WndProcHandler( hWnd, msg, wParam, lParam ) ) return true;
    switch ( msg )
    {
    case WM_CLOSE:
    case WM_DESTROY:
        if ( g_gui_instance ) g_gui_instance->shutdown( );
        PostQuitMessage( 0 );
        return 0;
    }
    return DefWindowProcW( hWnd, msg, wParam, lParam );
}

bool gui_t::initialize( )
{
    g_gui_instance = this;

    WNDCLASSEXW wc = { sizeof( WNDCLASSEXW ), CS_CLASSDC, WndProc, 0L, 0L,
                       GetModuleHandleW( nullptr ), nullptr, nullptr, nullptr,
                       nullptr, L"valcore_GUI", nullptr };
    ::RegisterClassExW( &wc );

    const int sw = ::GetSystemMetrics( SM_CXSCREEN );
    const int sh = ::GetSystemMetrics( SM_CYSCREEN );

    m_hwnd = ::CreateWindowExW( WS_EX_TOPMOST | WS_EX_LAYERED,
        wc.lpszClassName, L"valcore", WS_POPUP, 0, 0, sw, sh,
        nullptr, nullptr, wc.hInstance, nullptr );

    if ( !m_hwnd ) return false;

    constexpr MARGINS margins{ -1, -1, -1, -1 };
    ::DwmExtendFrameIntoClientArea( m_hwnd, &margins );
    ::SetLayeredWindowAttributes( m_hwnd, 0, 255, LWA_ALPHA );

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = sw; sd.BufferDesc.Height = sh;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60; sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = m_hwnd;
    sd.SampleDesc.Count = 1; sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    constexpr D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    D3D_FEATURE_LEVEL selected;
    if ( D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0,
             levels, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
             &g_pd3dDevice, &selected, &g_pd3dDeviceContext ) != S_OK )
        return false;

    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), (LPVOID*)&pBackBuffer );
    g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_mainRenderTargetView );
    pBackBuffer->Release( );

    IMGUI_CHECKVERSION( );
    ImGui::CreateContext( );
    ImGuiIO& io = ImGui::GetIO( );
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    ImGui_ImplWin32_Init( m_hwnd );
    ImGui_ImplDX11_Init( g_pd3dDevice, g_pd3dDeviceContext );

    ::ShowWindow( m_hwnd, SW_SHOWDEFAULT );
    ::UpdateWindow( m_hwnd );

    m_running = true;
    return true;
}

void gui_t::shutdown( )
{
    if ( m_host_running )
    {
        m_host_running = false;
        m_server.stop( );
        if ( m_host_thread.joinable( ) ) m_host_thread.join( );
    }
    if ( m_client_running )
    {
        m_client_running = false;
        m_socket.disconnect( );
        if ( m_client_thread.joinable( ) ) m_client_thread.join( );
    }

    ImGui_ImplDX11_Shutdown( );
    ImGui_ImplWin32_Shutdown( );
    ImGui::DestroyContext( );

    if ( g_mainRenderTargetView ) { g_mainRenderTargetView->Release( ); g_mainRenderTargetView = nullptr; }
    if ( g_pSwapChain )           { g_pSwapChain->Release( );           g_pSwapChain           = nullptr; }
    if ( g_pd3dDeviceContext )    { g_pd3dDeviceContext->Release( );    g_pd3dDeviceContext    = nullptr; }
    if ( g_pd3dDevice )           { g_pd3dDevice->Release( );           g_pd3dDevice           = nullptr; }
}

void gui_t::push_style( )
{
    ImGuiStyle& s = ImGui::GetStyle( );

    s.WindowRounding = 10.0f; s.ChildRounding  = 6.0f;
    s.FrameRounding  =  5.0f; s.PopupRounding  = 5.0f;
    s.TabRounding    =  5.0f; s.GrabRounding   = 3.0f;
    s.WindowBorderSize = 1.0f; s.FrameBorderSize = 0.0f; s.ChildBorderSize = 0.0f;

    s.WindowPadding    = { 12.0f,  8.0f };
    s.FramePadding     = {  8.0f,  4.0f };
    s.ItemSpacing      = {  6.0f,  6.0f };
    s.ItemInnerSpacing = {  4.0f,  4.0f };
    s.ScrollbarSize    = 8.0f;
    s.GrabMinSize      = 7.0f;

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]             = pal::bg_window;
    c[ImGuiCol_ChildBg]              = pal::bg_panel;
    c[ImGuiCol_PopupBg]              = pal::bg_window;
    c[ImGuiCol_Border]               = { 0.20f, 0.22f, 0.28f, 0.80f };
    c[ImGuiCol_BorderShadow]         = { 0, 0, 0, 0 };
    c[ImGuiCol_FrameBg]              = pal::bg_input;
    c[ImGuiCol_FrameBgHovered]       = { 0.18f, 0.18f, 0.24f, 1.0f };
    c[ImGuiCol_FrameBgActive]        = { 0.22f, 0.22f, 0.30f, 1.0f };
    c[ImGuiCol_TitleBg]              = pal::bg_panel;
    c[ImGuiCol_TitleBgActive]        = pal::bg_panel;
    c[ImGuiCol_TitleBgCollapsed]     = pal::bg_panel;
    c[ImGuiCol_ScrollbarBg]          = { 0, 0, 0, 0 };
    c[ImGuiCol_ScrollbarGrab]        = { 0.25f, 0.27f, 0.35f, 1.0f };
    c[ImGuiCol_ScrollbarGrabHovered] = { 0.32f, 0.34f, 0.44f, 1.0f };
    c[ImGuiCol_ScrollbarGrabActive]  = pal::accent;
    c[ImGuiCol_CheckMark]            = pal::accent;
    c[ImGuiCol_SliderGrab]           = pal::accent;
    c[ImGuiCol_SliderGrabActive]     = pal::accent_hov;
    c[ImGuiCol_Button]               = { 0.18f, 0.20f, 0.28f, 1.0f };
    c[ImGuiCol_ButtonHovered]        = { 0.25f, 0.40f, 0.80f, 1.0f };
    c[ImGuiCol_ButtonActive]         = pal::accent;
    c[ImGuiCol_Header]               = { 0.20f, 0.40f, 0.75f, 0.40f };
    c[ImGuiCol_HeaderHovered]        = { 0.25f, 0.45f, 0.85f, 0.60f };
    c[ImGuiCol_HeaderActive]         = pal::accent;
    c[ImGuiCol_Separator]            = { 0.20f, 0.22f, 0.28f, 0.80f };
    c[ImGuiCol_Tab]                  = { 0.12f, 0.13f, 0.17f, 1.0f };
    c[ImGuiCol_TabHovered]           = { 0.22f, 0.42f, 0.85f, 0.80f };
    c[ImGuiCol_TabActive]            = { 0.20f, 0.38f, 0.78f, 1.0f };
    c[ImGuiCol_TabUnfocused]         = { 0.12f, 0.13f, 0.17f, 1.0f };
    c[ImGuiCol_TabUnfocusedActive]   = { 0.16f, 0.28f, 0.58f, 1.0f };
    c[ImGuiCol_Text]                 = pal::text_white;
    c[ImGuiCol_TextDisabled]         = { 0.32f, 0.33f, 0.38f, 1.0f };
}

void gui_t::render_section_header( const char* label )
{
    ImGui::PushStyleColor( ImGuiCol_Text, pal::accent );
    ImGui::TextUnformatted( label );
    ImGui::PopStyleColor( );
    ImGui::PushStyleColor( ImGuiCol_Separator, pal::accent_dim );
    ImGui::Separator( );
    ImGui::PopStyleColor( );
}

void gui_t::render_status_badge( bool active )
{
    ImGui::PushStyleColor( ImGuiCol_Text, active ? pal::green : pal::text_dim );
    ImGui::TextUnformatted( active ? "RUNNING" : "STOPPED" );
    ImGui::PopStyleColor( );
}

static void draw_theme_line( ImDrawList* dl, ImVec2 wpos, float ww, float y )
{
    const float line_h = 2.0f;
    float time = (float)ImGui::GetTime( ) * 3.0f;

    auto get_theme_color = []( float offset ) -> ImU32
    {
        float sine = sinf( offset ) * 0.5f + 0.5f;

        int r = (int)( 46 + ( 100 - 46 ) * sine );
        int g = (int)( 97 + ( 200 - 97 ) * sine );
        int b = (int)( 184 + ( 255 - 184 ) * sine );

        return IM_COL32( r, g, b, 200 );
    };

    const int segments = 100;
    float seg_width = ww / segments;

    for ( int i = 0; i < segments; ++i )
    {
        float x1 = wpos.x + ww - ( i * seg_width );
        float x2 = wpos.x + ww - ( ( i + 1 ) * seg_width );

        float offset = time + ( i / (float)segments ) * 6.28f;

        ImU32 col = get_theme_color( offset );

        dl->AddRectFilled( ImVec2( x2, y ), ImVec2( x1, y + line_h ), col );
    }
}

void gui_t::render( )
{
    push_style( );

    ::SetWindowPos( m_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE );

    if ( GetAsyncKeyState( VK_INSERT ) & 1 )
    {
        m_menu_open = !m_menu_open;
        LONG_PTR ex = GetWindowLongPtrW( m_hwnd, GWL_EXSTYLE );
        if ( m_menu_open ) ex &= ~WS_EX_TRANSPARENT;
        else               ex |=  WS_EX_TRANSPARENT;
        SetWindowLongPtrW( m_hwnd, GWL_EXSTYLE, ex );
    }
    if ( GetAsyncKeyState( VK_END ) & 1 ) { m_running = false; return; }

    ImGui_ImplDX11_NewFrame( );
    ImGui_ImplWin32_NewFrame( );
    ImGui::NewFrame( );

    static ImVec2 win_pos = { 0, 0 };
    static bool   first   = true;
    if ( first )
    {
        ImVec2 ds = ImGui::GetIO( ).DisplaySize;
        win_pos   = { ( ds.x - 420.0f ) * 0.5f, ( ds.y - 370.0f ) * 0.5f };
        first     = false;
    }

    ImGui::SetNextWindowPos ( win_pos, ImGuiCond_Appearing );
    ImGui::SetNextWindowSize( { 420.0f, 380.0f }, ImGuiCond_Always );

    constexpr ImGuiWindowFlags WIN_FLAGS =
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoResize   | ImGuiWindowFlags_NoTitleBar  |
        ImGuiWindowFlags_NoScrollWithMouse;

    if ( m_menu_open && ImGui::Begin( "##valcore", &m_running, WIN_FLAGS ) )
    {
        if ( !ImGui::IsWindowAppearing( ) ) win_pos = ImGui::GetWindowPos( );

        ImDrawList* dl  = ImGui::GetWindowDrawList( );
        ImVec2 wpos     = ImGui::GetWindowPos( );
        float  ww       = ImGui::GetWindowWidth( );
        float  wh       = ImGui::GetWindowHeight( );

        const float bar_h = 28.0f;

        dl->AddRectFilled( wpos, ImVec2( wpos.x + ww, wpos.y + bar_h ),
            IM_COL32( 13, 13, 18, 255 ), 10.0f, ImDrawFlags_RoundCornersTop );

        draw_theme_line( dl, wpos, ww, wpos.y + bar_h );

        ImGui::SetCursorPos( { 0, 0 } );
        ImGui::InvisibleButton( "##drag", { ww, bar_h } );
        if ( ImGui::IsItemActive( ) && ImGui::IsMouseDragging( ImGuiMouseButton_Left ) )
        {
            ImVec2 d = ImGui::GetMouseDragDelta( ImGuiMouseButton_Left );
            win_pos.x += d.x; win_pos.y += d.y;
            ImGui::SetWindowPos( win_pos );
            ImGui::ResetMouseDragDelta( ImGuiMouseButton_Left );
        }

        ImGui::SetCursorPos( { 12.0f, 6.0f } );
        ImGui::PushStyleColor( ImGuiCol_Text, pal::accent );
        ImGui::TextUnformatted( "ValCpy" );
        ImGui::PopStyleColor( );

        ImGui::SameLine( ww - 88.0f );
        ImGui::SetCursorPosY( 7.5f );
        ImGui::PushStyleColor( ImGuiCol_Text, pal::text_dim );
        ImGui::TextUnformatted( "version 2.1" );
        ImGui::PopStyleColor( );

        ImGui::SetCursorPosY( bar_h + 2.0f );

        if ( ImGui::BeginTabBar( "##tabs" ) )
        {
            if ( ImGui::BeginTabItem( " Host " ) )   { render_host_tab( );   ImGui::EndTabItem( ); }
            if ( ImGui::BeginTabItem( " Client " ) )  { render_client_tab( ); ImGui::EndTabItem( ); }
            ImGui::EndTabBar( );
        }

        float footer_y = wh - 20.0f;
        dl->AddRectFilled(
            ImVec2( wpos.x, wpos.y + footer_y ),
            ImVec2( wpos.x + ww, wpos.y + wh ),
            IM_COL32( 10, 10, 13, 255 ), 10.0f, ImDrawFlags_RoundCornersBottom );

        ImGui::SetCursorPos( { 0, footer_y + 3.0f } );
        float tw = ImGui::CalcTextSize( "By Recso" ).x;
        ImGui::SetCursorPosX( ( ww - tw ) * 0.5f );
        ImGui::PushStyleColor( ImGuiCol_Text, pal::text_dim );
        ImGui::TextUnformatted( "By Recso" );
        ImGui::PopStyleColor( );

        ImGui::End( );
    }
    else if ( m_menu_open ) ImGui::End( );

    ImGui::Render( );
    float cc[4] = { 0, 0, 0, 0 };
    g_pd3dDeviceContext->OMSetRenderTargets( 1, &g_mainRenderTargetView, nullptr );
    g_pd3dDeviceContext->ClearRenderTargetView( g_mainRenderTargetView, cc );
    ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData( ) );
    g_pSwapChain->Present( 1, 0 );
}

void gui_t::render_host_tab( )
{
    ImGui::TextColored( pal::text_label, "Status" );
    ImGui::SameLine( 80.0f );
    render_status_badge( m_host_initialized );
    ImGui::Separator( );

    const float btn_w = 110.0f, btn_h = 24.0f;

    if ( !m_host_initialized )
    {
        ImGui::PushStyleColor( ImGuiCol_Button,        pal::accent_dim );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, pal::accent_hov );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive,  pal::accent );
        if ( ImGui::Button( "Start Host", { btn_w, btn_h } ) )
        {
            if ( !m_server.start( ) )
                add_log( "[!] Failed to start server", 12 );
            else
            {
                m_host_running = true;
                m_host_thread  = std::thread( &gui_t::host_thread_func, this );
                m_host_initialized = true;
                add_log( "[+] Host started on localhost:65433", 10 );
                add_log( "[*] Waiting for client connections...", 7 );
            }
        }
        ImGui::PopStyleColor( 3 );
    }
    else
    {
        ImGui::PushStyleColor( ImGuiCol_Button,        { 0.30f, 0.12f, 0.12f, 1.0f } );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, { 0.55f, 0.15f, 0.15f, 1.0f } );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive,  pal::red );
        if ( ImGui::Button( "Stop Host", { btn_w, btn_h } ) )
        {
            m_host_running = false;
            m_server.stop( );
            if ( m_host_thread.joinable( ) ) m_host_thread.join( );
            m_host_initialized = false;
            add_log( "[!] Host stopped", 14 );
        }
        ImGui::PopStyleColor( 3 );
    }

    render_section_header( "Console" );

    ImGui::BeginDisabled( !m_host_initialized );
    render_console( );
    ImGui::EndDisabled( );
}

void gui_t::render_client_tab( )
{
    ImGui::TextColored( pal::text_label, "Status" );
    ImGui::SameLine( 80.0f );
    render_status_badge( m_client_initialized );
    ImGui::Separator( );

    const float btn_w = 110.0f, btn_h = 24.0f;

    if ( !m_client_initialized )
    {
        ImGui::PushStyleColor( ImGuiCol_Button,        pal::accent_dim );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, pal::accent_hov );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive,  pal::accent );
        if ( ImGui::Button( "Connect", { btn_w, btn_h } ) )
        {
            if ( !m_config.load( "config.ini" ) )
            {
                add_log( "[!] Config not found, creating default", 14 );
                m_config.save( "config.ini" );
                add_log( "[+] Default config created", 10 );
            }

            if ( !m_capture.initialize( m_config.scan_area, m_config.scan_area ) )
                add_log( "[!] Failed to initialize screen capture", 12 );
            else if ( !m_socket.connect( "localhost", 65433 ) )
                add_log( "[!] Failed to connect to host", 12 );
            else
            {
                m_client_running     = true;
                m_client_thread      = std::thread( &gui_t::client_thread_func, this );
                m_client_initialized = true;
                add_log( "[+] Client initialized", 10 );
                add_log( "[+] Connected to host", 10 );
            }
        }
        ImGui::PopStyleColor( 3 );
    }
    else
    {
        ImGui::PushStyleColor( ImGuiCol_Button,        { 0.30f, 0.12f, 0.12f, 1.0f } );
        ImGui::PushStyleColor( ImGuiCol_ButtonHovered, { 0.55f, 0.15f, 0.15f, 1.0f } );
        ImGui::PushStyleColor( ImGuiCol_ButtonActive,  pal::red );
        if ( ImGui::Button( "Disconnect", { btn_w, btn_h } ) )
        {
            m_client_running = false;
            m_socket.disconnect( );
            if ( m_client_thread.joinable( ) ) m_client_thread.join( );
            m_client_initialized = false;
            add_log( "[!] Client stopped", 14 );
        }
        ImGui::PopStyleColor( 3 );
    }

    ImGui::BeginDisabled( !m_client_initialized );

    render_section_header( "Settings" );

    const float lw = 90.0f;

    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, { 6.0f, 7.0f } );

    ImGui::TextColored( pal::text_label, "Key" );
    ImGui::SameLine( lw );
    if ( m_waiting_for_key )
    {
        ImGui::TextColored( pal::yellow, "Press any key..." );
        if ( m_client_initialized )
        {
            for ( int i = 1; i < 256; ++i )
            {
                if ( GetAsyncKeyState( i ) & 0x8000 )
                {
                    m_config.hold_key = i;
                    m_waiting_for_key = false;
                    m_config.save( "config.ini" );
                    break;
                }
            }
        }
    }
    else
    {
        std::string kn = utilities::get_key_name( m_config.hold_key );
        ImGui::TextColored( pal::text_white, "%s", kn.c_str( ) );
        ImGui::SameLine( );
        if ( ImGui::SmallButton( "Change" ) ) m_waiting_for_key = true;
    }

    ImGui::TextColored( pal::text_label, "Delay" );
    ImGui::SameLine( lw );
    ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail( ).x );
    const char* delay_labels[] = { "Slow (500ms)", "Medium (200ms)", "Fast (0ms)" };
    if ( ImGui::Combo( "##delay", &m_delay_mode, delay_labels, 3 ) )
        m_config.save( "config.ini" );

    ImGui::TextColored( pal::text_label, "Color Sens" );
    ImGui::SameLine( lw );
    ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail( ).x );
    ImGui::SliderInt( "##sens", &m_config.color_sens, 1, 100 );
    if ( ImGui::IsItemDeactivated( ) ) m_config.save( "config.ini" );

    ImGui::TextColored( pal::text_label, "FOV" );
    ImGui::SameLine( lw );
    ImGui::SetNextItemWidth( ImGui::GetContentRegionAvail( ).x );
    ImGui::BeginDisabled( true );
    ImGui::SliderInt( "##fov", &m_config.scan_area, 1, 50 );
    ImGui::EndDisabled( );

    ImGui::PopStyleVar( );

    render_section_header( "Config" );

    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, { 6.0f, 5.0f } );

    auto row = [&]( const char* k, const char* fmt, auto... a )
    {
        ImGui::TextColored( pal::text_label, "%s", k );
        ImGui::SameLine( lw );
        ImGui::TextColored( pal::text_dim, fmt, a... );
    };
    row( "Key",  "%d",               m_config.hold_key );
    row( "FOV",  "%d px",            m_config.scan_area );
    row( "Sens", "%d",               m_config.color_sens );
    row( "HSV+", "[%.0f,%.0f,%.0f]", (float)m_config.hsv_upper[0], (float)m_config.hsv_upper[1], (float)m_config.hsv_upper[2] );
    row( "HSV-", "[%.0f,%.0f,%.0f]", (float)m_config.hsv_lower[0], (float)m_config.hsv_lower[1], (float)m_config.hsv_lower[2] );

    ImGui::PopStyleVar( );

    ImGui::EndDisabled( );

    // Add bottom padding to prevent collision with footer bar
    ImGui::Dummy( { 0, 28.0f } );
}

void gui_t::render_console( )
{
    ImGui::Checkbox( "Auto-scroll", &m_auto_scroll );
    ImGui::SameLine( );
    ImGui::PushStyleColor( ImGuiCol_Button,        { 0.18f, 0.18f, 0.24f, 1.0f } );
    ImGui::PushStyleColor( ImGuiCol_ButtonHovered, { 0.25f, 0.25f, 0.34f, 1.0f } );
    if ( ImGui::SmallButton( "Clear" ) ) m_console_logs.clear( );
    ImGui::PopStyleColor( 2 );

    float avail_h = ImGui::GetContentRegionAvail( ).y - 22.0f;
    ImGui::PushStyleColor( ImGuiCol_ChildBg, { 0.06f, 0.06f, 0.08f, 1.0f } );
    ImGui::BeginChild( "##log", { 0, avail_h }, false );
    ImGui::PushStyleVar( ImGuiStyleVar_ItemSpacing, { 4.0f, 2.0f } );

    for ( const auto& log : m_console_logs )
    {
        ImVec4 col;
        switch ( log.color )
        {
        case 10: col = pal::green;  break;
        case 12: col = pal::red;    break;
        case 14: col = pal::yellow; break;
        default: col = pal::text_dim; break;
        }
        ImGui::TextColored( col, "%s", log.message.c_str( ) );
    }

    ImGui::PopStyleVar( );
    if ( m_auto_scroll && ImGui::GetScrollY( ) >= ImGui::GetScrollMaxY( ) )
        ImGui::SetScrollHereY( 1.0f );

    ImGui::EndChild( );
    ImGui::PopStyleColor( );
}

void gui_t::host_thread_func( )
{
    m_server.run_loop( );
}

void gui_t::client_thread_func( )
{
    auto   last_fire = std::chrono::steady_clock::now( );
    double delays[]  = { 0.5, 0.2, 0.0 };

    while ( m_client_running )
    {
        if ( GetAsyncKeyState( m_config.hold_key ) & 0x8000 )
        {
            auto   now     = std::chrono::steady_clock::now( );
            double elapsed = std::chrono::duration<double>( now - last_fire ).count( );
            double rate    = delays[ m_delay_mode ];

            if ( rate == 0.0 || elapsed >= rate )
            {
                pixel_data_t frame = m_capture.capture( );
                if ( frame.data && m_detector.detect_red( frame, m_config ) )
                {
                    if ( !m_socket.send_click( ) )
                    {
                        add_log( "[!] Connection lost, reconnecting...", 14 );
                        m_socket.disconnect( );
                        while ( m_client_running && !m_socket.connect( "localhost", 65433 ) )
                            std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
                        if ( m_client_running ) add_log( "[+] Reconnected", 10 );
                    }
                    last_fire = now;
                }
            }
        }
        std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
    }
}

void gui_t::add_log( const std::string& message, int color )
{
    m_console_logs.push_back( { message, color } );
}