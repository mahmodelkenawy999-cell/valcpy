#include <stdafx.hpp>
#include "dtc.hpp"

void detector_t::rgb_to_hsv( uint8_t r, uint8_t g, uint8_t b, float& h, float& s, float& v )
{
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;

    float max_val = ( std::max )( { rf, gf, bf } );
    float min_val = ( std::min )( { rf, gf, bf } );
    float diff = max_val - min_val;

    v = max_val;

    if ( diff < 0.0001f )
    {
        h = 0;
        s = 0;
        return;
    }

    s = diff / max_val;

    if ( max_val == rf )
        h = 60.0f * fmodf( ( gf - bf ) / diff + 6.0f, 6.0f );
    else if ( max_val == gf )
        h = 60.0f * ( ( bf - rf ) / diff + 2.0f );
    else
        h = 60.0f * ( ( rf - gf ) / diff + 4.0f );

    if ( h < 0 )
        h += 360.0f;
}

bool detector_t::detect_red( const pixel_data_t& frame, const config_t& cfg )
{
    if ( !frame.data || frame.width <= 0 || frame.height <= 0 )
        return false;

    int total_pixels = frame.width * frame.height;
    int red_count = 0;

    uint8_t* pixels = frame.data;

    for ( int y = 0; y < frame.height; ++y )
    {
        for ( int x = 0; x < frame.width; ++x )
        {
            int idx = ( y * frame.width + x ) * 3;
            uint8_t r = pixels[ idx + 0 ];
            uint8_t g = pixels[ idx + 1 ];
            uint8_t b = pixels[ idx + 2 ];
            if ( r <= g + 30 || r <= b + 30 || r <= 100 )
                continue;
            float h, s, v;
            rgb_to_hsv( r, g, b, h, s, v );

            bool in_range1 = ( h >= cfg.hsv_lower[ 0 ] && h <= cfg.hsv_upper[ 0 ] ) &&
                             ( s >= cfg.hsv_lower[ 1 ] / 255.0f && s <= cfg.hsv_upper[ 1 ] / 255.0f ) &&
                             ( v >= cfg.hsv_lower[ 2 ] / 255.0f && v <= cfg.hsv_upper[ 2 ] / 255.0f );

            bool in_range2 = ( h >= cfg.hsv_lower2[ 0 ] && h <= cfg.hsv_upper2[ 0 ] ) &&
                             ( s >= cfg.hsv_lower2[ 1 ] / 255.0f && s <= cfg.hsv_upper2[ 1 ] / 255.0f ) &&
                             ( v >= cfg.hsv_lower2[ 2 ] / 255.0f && v <= cfg.hsv_upper2[ 2 ] / 255.0f );

            if ( in_range1 || in_range2 )
                red_count++;
        }
    }

    int threshold = ( std::max )( 1, static_cast< int >( total_pixels * ( cfg.color_sens / 1000.0 ) ) );
    return red_count >= threshold;
}
