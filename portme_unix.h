// Sets up any platform-specific facilities.
void init( void )
{
}

// Prints the null-terminated ASCII string `s` to a TTY somewhere.
#include <stdio.h>
void print( const char * s )
{
	printf( "%s\n", s );
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

// Blocks until a button is pressed down somewhere, then returns that movement.
#define _XOPEN_SOURCE 500
#include <unistd.h>
enum move getmove( void )
{
	usleep( 1000000 ); // 1 sec
}


