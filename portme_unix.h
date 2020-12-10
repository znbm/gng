// Sets up any platform-specific facilities.
void init( void )
{
}

// Prints the null-terminated ASCII string `s` to a TTY somewhere.
#include <stdio.h>
void print( const char * s )
{
	printf( "%s", s );
}

// Seeds the PRNG.
#include <time.h>
void seed()
{
	for ( volatile int v = 0; v < 100; v++ ); // wait so `time` doesn't yield zero
	rng_a = time( NULL );
	rng_b = time( NULL );
	rng_c = time( NULL );
	for ( volatile int i = 0; i < 10; i++ ) rng32(); // burn subpar values
}

// Blocks until a button is pressed down somewhere, then returns that movement.
enum move getmove( void )
{
	char e = getchar();

	switch ( e )
	{
		case 'w': case 'k': return MOVE_UP;
		case 'a': case 'h': return MOVE_LEFT;
		case 's': case 'j': return MOVE_DOWN;
		case 'd': case 'l': return MOVE_RIGHT;
		case 'q': return MOVE_QUIT;
		default:  return MOVE_BAD;
	}
}


