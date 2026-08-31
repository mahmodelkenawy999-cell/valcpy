#pragma once

struct pixel_data_t
{
    uint8_t* data = nullptr;
    int width = 0;
    int height = 0;
    int channels = 3;
};

class screen_capture_t
{
public:
    ~screen_capture_t( );

    bool initialize( int width, int height );
    pixel_data_t capture( );

    int get_width( ) const { return m_width; }
    int get_height( ) const { return m_height; }

private:
    int m_width = 0;
    int m_height = 0;
    HDC m_screen_dc = nullptr;
    HDC m_mem_dc = nullptr;
    HBITMAP m_bitmap = nullptr;
    void* m_bits = nullptr;
    std::vector<uint8_t> m_rgb_buffer;

    bool m_initialized = false;
};
