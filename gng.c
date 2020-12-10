#include <stdint.h>

// The dungeon is a 32x32 tilemap.
#define TILE_FLOOR 0
#define TILE_WALL 1
#define TILE_WATER 2
#define TILE_LAVA 3
#define TILE_ACID 4
unsigned char tilemap[ 32 ][ 32 ]; // zeroed, i.e. filled with floor

enum move
{
	MOVE_UP,
	MOVE_DOWN,
	MOVE_LEFT,
	MOVE_RIGHT,
	MOVE_QUIT
};

// Generates a pseudorandom 32-bit bit value.
// Implements the public-domain 'sfc32' PRNG from
// version 0.94 of Chris Doty-Humphrey's PractRand. 
static uint32_t random_a, random_b, random_c, random_ctr = 1; // global to allow seeding
uint32_t random32( void )
{
	uint32_t r = random_a + random_b + random_ctr;
	
	random_a = random_b ^ ( random_b >> 9 );
	random_b = random_c + ( random_c << 3 );
	random_c = ( ( random_c << 21 ) | ( random_c >> ( 32 - 21 ) ) ) + r;

	random_ctr += 1;
	return r;
}

// Debiased modulo method for an unbiased random number between `lo` inclusive and `hi` exclusive.
uint32_t random( uint32_t lo, uint32_t hi )
{
	uint32_t range = hi - lo;
	
	uint32_t x, r;
	do
	{
		x = random32();
		r = x % range;
	}
	while ( x - r > -range );
	
	return r + lo;
}

// Nonportable utilities.
#include "portme.h"

struct rect
{
	unsigned int tlx, tly;
	unsigned int brx, bry;
};

// Generates the dungeon tilemap.
void dungen()
{
	// A full, "heap-style" binary tree representing our bsp rectangles.
	struct rect b[ 1 + 1 + 2 + 4 + 8 + 16 ];

	// Initialize the tree to depth 1. To simplify the math, entry zero is unused.
	b[ 0 ].tlx = b[ 0 ].tly = b[ 0 ].brx = b[ 0 ].bry = 0;
	b[ 1 ].tlx = b[ 1 ].tly = 1;
	b[ 1 ].brx = b[ 1 ].bry = 30;

	// Draw exterior wall.
	for ( unsigned int i = 0; i < 32; i++ )
	{
		tilemap[ i ][ 0 ] = TILE_WALL;
		tilemap[ i ][ 31 ] = TILE_WALL;
		tilemap[ 0 ][ i ] = TILE_WALL;
		tilemap[ 31 ][ i ]= TILE_WALL;
	}

	// Create interior walls.
	for ( unsigned int i = 1; i < 1 + 2 + 4 + 8; i++ )
	{
		// Height and width of the current rectangle.
		unsigned int w = b[ i ].brx - b[ i ].tlx;
		unsigned int h = b[ i ].bry - b[ i ].tly;

		// Whether to make a horizontal (0) or vertical (1) cut.
		_Bool vcut;
		const unsigned int SMALLRECT = 7;
		if ( h <= SMALLRECT && w <= SMALLRECT ) // our rectangle is too small, stop slicing
		{
			b[ i ].tlx = b[ i ].tly = b[ i ].brx = b[ i ].bry = 0;
			continue;
		}
		else if ( h <= SMALLRECT ) vcut = 1;
		else if ( w <= SMALLRECT ) vcut = 0; 
		else vcut = random( 0, h + w ) < w; // random, but biased towards making squares

		// Choose a random cut posititon withing the innermost 1/2 of the rectangle.
		unsigned int lo = vcut ? b[ i ].tlx + w / 4 : b[ i ].tly + h / 4;
		unsigned int hi = vcut ? b[ i ].brx - w / 4 : b[ i ].bry - h / 4;
		unsigned int r = random( lo, hi );

		// Draw the cut.
		if ( vcut ) for ( unsigned int j = b[ i ].tly; j <= b[ i ].bry; j++ ) tilemap[ r ][ j ] = TILE_WALL;
		else for ( unsigned int j = b[ i ].tlx; j <= b[ i ].brx; j++ ) tilemap[ j ][ r ] = TILE_WALL;

		// Two children always inheirit some of their parents' dimensions...
		b[ 2 * i ].tlx = b[ i ].tlx; 
		b[ 2 * i ].tly = b[ i ].tly;
		b[ 2 * i + 1 ].brx = b[ i ].brx;
		b[ 2 * i + 1 ].bry = b[ i ].bry;
	
		// ...but might not inheirit some others.
		b[ 2 * i ].brx = vcut ? r - 1 : b[ i ].brx;
		b[ 2 * i ].bry = vcut ? b[ i ].bry : r - 1;
		b[ 2 * i + 1 ].tlx = vcut ? r + 1 : b[ i ].tlx; 
		b[ 2 * i + 1 ].tly = vcut ? b[ i ].tly : r + 1;
	}

	// Create doorways.
	/*
	for ( unsigned int i = 1 + 2 + 4 + 8 - 1; i >= 2; i-- )
	{
		if ( b[ i ].tlx != b[ i / 2 ].tlx ) tilemap[  ]
		{

		}
		else if ( b[ i ].brx != b[ i / 2 ].brx ) // b[ i ] was formed by a vertical cut
	}
	*/
}

// Moves the player, advances the game, and updates the screen.
unsigned int px, py; // player coordinates
void move( enum move m )
{
	     if ( m == MOVE_UP )    print( "up\n\r" ); // py++; 
	else if ( m == MOVE_DOWN )  print( "down\n\r" ); // py--;
	else if ( m == MOVE_LEFT )  print( "left\n\r" ); // px--;
	else if ( m == MOVE_RIGHT ) print( "right\n\r" ); // px++;

}

#include <stdio.h>
int main( void )
{
	init();
	seed();
	dungen();

	for ( int i = 0; i < 32; i++ )
	{
		for ( int j = 0; j < 32; j++ )
		{
			switch ( tilemap[ j ][ i ] )
			{
				case TILE_FLOOR: print( "\033[30;100m  " ); break;
				case TILE_WALL:  print( "\033[37;107m::" ); break;
				case TILE_WATER: print( "\033[36;104m~~" ); break;
				case TILE_LAVA:  print( "\033[93;101m~~" ); break;
				case TILE_ACID:  print( "\033[33;42m~~" );  break;
			}
		}
		print( "\033[0m\n" );
	}

	unsigned int px = 2, py = 2;

	for ( ;; )
	{
		move( getmove() );
	}
}


