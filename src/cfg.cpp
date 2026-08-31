#include <stdafx.hpp>
#include "cfg.hpp"

bool config_t::load( const std::string& filepath )
{
    std::ifstream file( filepath );
    if ( !file.is_open( ) )
        return false;

    std::string line, current_section;
    while ( std::getline( file, line ) )
    {
        auto start = line.find_first_not_of( " \t" );
        if ( start == std::string::npos )
            continue;
        auto end = line.find_last_not_of( " \t" );
        line = line.substr( start, end - start + 1 );

        if ( line.empty( ) || line[ 0 ] == ';' || line[ 0 ] == '#' )
            continue;

        if ( line.front( ) == '[' && line.back( ) == ']' )
        {
            current_section = line.substr( 1, line.size( ) - 2 );
            continue;
        }

        auto eq_pos = line.find( '=' );
        if ( eq_pos == std::string::npos )
            continue;

        std::string key = line.substr( 0, eq_pos );
        std::string value = line.substr( eq_pos + 1 );

        key = key.substr( key.find_first_not_of( " \t" ) );
        key = key.substr( 0, key.find_last_not_of( " \t" ) + 1 );
        value = value.substr( value.find_first_not_of( " \t" ) );
        value = value.substr( 0, value.find_last_not_of( " \t" ) + 1 );

        try
        {
            if ( current_section == "Settings" )
            {
                if ( key == "key" ) hold_key = std::stoi( value );
                else if ( key == "fov" ) scan_area = std::stoi( value );
            }
            else if ( current_section == "HSV" )
            {
                if ( key == "color_sens" ) color_sens = std::stoi( value );
                else if ( key.find( "lower" ) == 0 || key.find( "upper" ) == 0 )
                {
                    auto arr_start = value.find( '[' );
                    auto arr_end = value.find( ']' );
                    if ( arr_start != std::string::npos && arr_end != std::string::npos )
                    {
                        std::string arr_content = value.substr( arr_start + 1, arr_end - arr_start - 1 );
                        std::stringstream ss( arr_content );
                        std::string item;
                        int idx = 0;
                        std::array<int, 3> arr{};
                        while ( std::getline( ss, item, ',' ) && idx < 3 )
                        {
                            arr[ idx++ ] = std::stoi( item );
                        }

                        if ( key == "lower" ) hsv_lower = arr;
                        else if ( key == "upper" ) hsv_upper = arr;
                        else if ( key == "lower2" ) hsv_lower2 = arr;
                        else if ( key == "upper2" ) hsv_upper2 = arr;
                    }
                }
            }
        }
        catch ( ... ) {}
    }

    return true;
}

void config_t::save( const std::string& filepath ) const
{
    std::ofstream file( filepath );
    if ( !file.is_open( ) )
        return;

    file << "[Settings]\n";
    file << "key = " << hold_key << "\n";
    file << "fov = " << scan_area << "\n\n";

    file << "[HSV]\n";
    file << "upper = [" << hsv_upper[ 0 ] << ", " << hsv_upper[ 1 ] << ", " << hsv_upper[ 2 ] << "]\n";
    file << "lower = [" << hsv_lower[ 0 ] << ", " << hsv_lower[ 1 ] << ", " << hsv_lower[ 2 ] << "]\n";
    file << "upper2 = [" << hsv_upper2[ 0 ] << ", " << hsv_upper2[ 1 ] << ", " << hsv_upper2[ 2 ] << "]\n";
    file << "lower2 = [" << hsv_lower2[ 0 ] << ", " << hsv_lower2[ 1 ] << ", " << hsv_lower2[ 2 ] << "]\n";
    file << "color_sens = " << color_sens << "\n";
}
