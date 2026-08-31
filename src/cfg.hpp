#pragma once

struct config_t
{
    int hold_key = 0xA4;
    int scan_area = 6;

    std::array<int, 3> hsv_lower = { 0, 150, 150 };
    std::array<int, 3> hsv_upper = { 5, 255, 255 };
    std::array<int, 3> hsv_lower2 = { 170, 150, 150 };
    std::array<int, 3> hsv_upper2 = { 180, 255, 255 };

    int color_sens = 0;

    bool load( const std::string& filepath );
    void save( const std::string& filepath ) const;
};
