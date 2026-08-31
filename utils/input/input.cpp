#include <stdafx.hpp>
#include "input.hpp"

bool input::initialize( )
{
	return true;
}

void input::inject_mouse( int x, int y, std::uint8_t buttons ) const
{
	HWND hwnd = GetForegroundWindow( );
	if ( !hwnd )
		return;

	LPARAM lParam = ( y << 16 ) | ( x & 0xFFFF );

	if ( buttons & mouse_buttons::left_down )
	{
		PostMessageW( hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam );
	}
	else if ( buttons & mouse_buttons::left_up )
	{
		PostMessageW( hwnd, WM_LBUTTONUP, 0, lParam );
	}
	else if ( buttons & mouse_buttons::right_down )
	{
		PostMessageW( hwnd, WM_RBUTTONDOWN, MK_RBUTTON, lParam );
	}
	else if ( buttons & mouse_buttons::right_up )
	{
		PostMessageW( hwnd, WM_RBUTTONUP, 0, lParam );
	}
}