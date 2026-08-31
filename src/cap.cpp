#include <stdafx.hpp>
#include "cap.hpp"

screen_capture_t::~screen_capture_t( )
{
    if ( m_mem_dc )
        DeleteDC( m_mem_dc );
    if ( m_bitmap )
        DeleteObject( m_bitmap );
    if ( m_screen_dc )
        ReleaseDC( nullptr, m_screen_dc );
}

bool screen_capture_t::initialize( int width, int height )
{
    m_width = width;
    m_height = height;

    m_screen_dc = GetDC( nullptr );
    if ( !m_screen_dc )
        return false;

    m_mem_dc = CreateCompatibleDC( m_screen_dc );
    if ( !m_mem_dc )
        return false;

    BITMAPINFO bmi{};
    bmi.bmiHeader.biSize = sizeof( BITMAPINFOHEADER );
    bmi.bmiHeader.biWidth = width;
    bmi.bmiHeader.biHeight = -height;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    m_bitmap = CreateDIBSection( m_screen_dc, &bmi, DIB_RGB_COLORS, &m_bits, nullptr, 0 );
    if ( !m_bitmap )
        return false;

    SelectObject( m_mem_dc, m_bitmap );

    m_rgb_buffer.resize( width * height * 3 );
    m_initialized = true;
    return true;
}

pixel_data_t screen_capture_t::capture( )
{
    pixel_data_t result{};
    if ( !m_initialized )
        return result;

    int screen_width = GetSystemMetrics( SM_CXSCREEN );
    int screen_height = GetSystemMetrics( SM_CYSCREEN );
    int x = ( screen_width - m_width ) / 2;
    int y = ( screen_height - m_height ) / 2;

    BitBlt( m_mem_dc, 0, 0, m_width, m_height, m_screen_dc, x, y, SRCCOPY );

    uint8_t* src = static_cast< uint8_t* >( m_bits );
    for ( int i = 0; i < m_width * m_height; ++i )
    {
        m_rgb_buffer[ i * 3 + 0 ] = src[ i * 4 + 2 ];
        m_rgb_buffer[ i * 3 + 1 ] = src[ i * 4 + 1 ];
        m_rgb_buffer[ i * 3 + 2 ] = src[ i * 4 + 0 ];
    }

    result.data = m_rgb_buffer.data( );
    result.width = m_width;
    result.height = m_height;
    result.channels = 3;
    return result;
}
