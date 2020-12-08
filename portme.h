// Sets up any platform-specific facilities.
void init( void )
{
}

// Seeds the PRNG.
void seed()
{
	random_a = 0;
	random_b = 0;
	random_c = 0;
	for ( volatile int i = 0; i < 10; i++ ) random32(); // burn subpar values
}

// Prints the null-terminated ASCII string `s` to a TTY somewhere.
void print( const char * s )
{
}

// Blocks until a button is pressed down somewhere, then returns that movement.
enum { MOVE_UP, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT } getmove( void )
{
}



