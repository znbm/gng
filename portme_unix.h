// Sets up any platform-specific facilities.
void init( void )
{
}

// Seeds the PRNG.
#include <time.h>
void seed()
{
	for ( volatile int v = 0; v < 100; v++ ); // wait so `time` doesn't yield zero
	random_a = time( NULL );
	random_b = time( NULL );
	random_c = time( NULL );
	for ( volatile int i = 0; i < 10; i++ ) random32(); // burn subpar values
}

// Prints the null-terminated ASCII string `s` to a TTY somewhere.
#include <stdio.h>
void print( const char * s )
{
	printf( "%s\n", s );
}

// Blocks until a button is pressed down somewhere, then returns that movement.
enum { MOVE_UP, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT } getmove( void )
{
}



