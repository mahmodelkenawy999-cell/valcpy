#pragma once

class input
{
public:
	bool initialize( );
	void inject_mouse( int x, int y, std::uint8_t buttons ) const;

	enum mouse_buttons : std::uint8_t
	{
		none = 0,
		left_down = 1 << 0,
		left_up = 1 << 1,
		right_down = 1 << 2,
		right_up = 1 << 3,
		move = 1 << 4
	};
};