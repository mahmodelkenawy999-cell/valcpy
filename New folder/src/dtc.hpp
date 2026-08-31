#pragma once
#include "cfg.hpp"
#include "cap.hpp"

class detector_t
{
public:
    bool detect_red( const pixel_data_t& frame, const config_t& cfg );

private:
    static void rgb_to_hsv( uint8_t r, uint8_t g, uint8_t b, float& h, float& s, float& v );
};
