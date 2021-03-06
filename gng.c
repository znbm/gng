#include <stdint.h>

// The dungeon is a 32x32 tilemap.
enum tile
{
	TILE_FLOOR,
	TILE_WALL,

	// Hazards
	TILE_WATER = 32,
	TILE_LAVA,
	TILE_ACID,
	TILE_SPIKES,
	TILE_HOLE,

	TILE_MONIES = 64,

	TILE_PLAYER = 128 // only for drawing
};
unsigned char tilemap[ 32 ][ 32 ]; // zeroed, i.e. filled with floor

enum move
{
	MOVE_UP,
	MOVE_DOWN,
	MOVE_LEFT,
	MOVE_RIGHT,
	MOVE_QUIT,
	MOVE_BAD
};

unsigned int px, py; // player coordinates

// Generates a pseudorandom 32-bit bit value.
// Implements the public-domain 'sfc32' PRNG from
// version 0.94 of Chris Doty-Humphrey's PractRand. 
static uint32_t rng_a, rng_b, rng_c, rng_ctr = 1; // global to allow seeding
uint32_t rng32( void )
{
	uint32_t r = rng_a + rng_b + rng_ctr;
	
	rng_a = rng_b ^ ( rng_b >> 9 );
	rng_b = rng_c + ( rng_c << 3 );
	rng_c = ( ( rng_c << 21 ) | ( rng_c >> ( 32 - 21 ) ) ) + r;

	rng_ctr += 1;
	return r;
}

// Debiased modulo method for an unbiased random number between `lo` inclusive and `hi` exclusive.
uint32_t rng( uint32_t lo, uint32_t hi )
{
	uint32_t range = hi - lo;
	
	uint32_t x, r;
	do
	{
		x = rng32();
		r = x % range;
	}
	while ( x - r > -range );
	
	return r + lo;
}

// Returns a pointer to a null-terminated string containing `n` written in decimal.
// If `n` == 0, the string is "0"; otherwise, it has no leading zeroes.
char * uintstr( uint32_t n )
{
	static char s[ 11 ] = { '\0' }; // 11 bytes can store "4294967295\0"

	// Precompute the number of digits in `n` so we can avoid leading zeroes.
	int len = 1;
	for ( int ncpy = n / 10u; ncpy > 0u; len++ ) ncpy /= 10u;
	
	// Populate the first `len` characters with `n`'s digits.
	for ( int i = len - 1; i >= 0; i-- )
	{
		s[ i ] = '0' + n % 10u;
		n /= 10u;
	}

	// Nullify the rest of the string.
	for ( int i = 10; i >= len; i-- ) s[ i ] = '\0';

	return s;
}

// Nonportable utilities.
#include "portme_unix.h"

struct rect
{
	unsigned int tlx, tly;
	unsigned int brx, bry;
};

// Generates the dungeon tilemap.
void dungen()
{
	// A full, "heap-style" binary tree representing our bsp rectangles.
	struct rect b[ 1 + 1 + 2 + 4 + 8 + 16 + 10 ]; // i don't even know how many entries this has anymore

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
	for ( unsigned int i = 1; i <= 1 + 2 + 4 + 8; i++ )
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
		else vcut = rng( 0, h + w ) < w; // random, but biased towards making squares

		// Choose a random cut posititon withing the innermost 1/2 of the rectangle.
		unsigned int lo = vcut ? b[ i ].tlx + w / 4 : b[ i ].tly + h / 4;
		unsigned int hi = vcut ? b[ i ].brx - w / 4 : b[ i ].bry - h / 4;
		unsigned int r = rng( lo, hi );

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

	// Create pools of hazards.
	for ( unsigned int i = 1 + 1 + 2 + 4 + 8; i < 1 + 1 + 2 + 4 + 8 + 16; i++ ) // for every leaf / individual room...
	{
		enum tile liquid = (enum tile)( rng( 32, 32 + 5 ) ); // pick a random hazard

		if ( b[ i / 2 ].tlx == 0 ) continue; // nonexistent room, skip it

		// Calculate pool sizes.
		unsigned int ptlx = rng( b[ i ].tlx, b[ i ].brx );
		unsigned int pbrx = rng( b[ i ].tlx, b[ i ].brx );
		unsigned int ptly = rng( b[ i ].tly, b[ i ].bry );
		unsigned int pbry = rng( b[ i ].tly, b[ i ].bry );

		// If our random bottom rights are before our top lefts,
		// or are even close to being so,
		// just fill the whole dimension / room.
		if ( pbrx < ptlx + 1 )
		{
			ptlx = b[ i ].tlx;
			pbrx = b[ i ].brx;
		}
		if ( pbry < ptly + 1 )
		{
			ptly = b[ i ].tly;
			pbry = b[ i ].bry;
		}

		// Draw the pools.
		for ( unsigned int ix = ptlx; ix <= pbrx; ix++ )
		for ( unsigned int iy = ptly; iy <= pbry; iy++ )
		{
			tilemap[ ix ][ iy ] = liquid;
		}
	}

	// Create doorways.
	for ( unsigned int i = 1 + 1 + 2 + 4 + 8 + 16 - 1; i > 1 + 1; i -= 2 ) // traverse back up the tree by pairs
	{
		if ( b[ i ].tlx == 0 || b[ i - 1 ].tlx == 0 ) continue; // nonexistent room, skip it

		// Figure out which wall the room shares with its tree neighbor.
		// Then, pick a doorway and draw a catwalk over any hazards.
		if ( b[ i ].tly == b[ i - 1 ].bry + 2 ) // shared north wall
		{
			unsigned int d = rng( b[ i ].tlx, b[ i ].brx );
			for ( unsigned int cwi = b[ i - 1 ].tly; cwi <= b[ i ].bry; cwi++ ) tilemap[ d ][ cwi ] = TILE_FLOOR;
		}
		if ( b[ i ].bry == b[ i - 1 ].tly - 2 ) // shared south wall
		{
			unsigned int d = rng( b[ i ].tlx, b[ i ].brx );
			for ( unsigned int cwi = b[ i ].tly; cwi <= b[ i - 1 ].bry; cwi++ ) tilemap[ d ][ cwi ] = TILE_FLOOR;
		}
		if ( b[ i ].tlx == b[ i - 1 ].brx + 2 ) // shared east wall
		{
			unsigned int d = rng( b[ i ].tly, b[ i ].bry );
			for ( unsigned int cwi = b[ i - 1 ].tlx; cwi <= b[ i ].brx; cwi++ ) tilemap[ cwi ][ d ] = TILE_FLOOR;
		}
		if ( b[ i ].brx == b[ i - 1 ].tlx - 2 ) // shared west wall
		{
			unsigned int d = rng( b[ i ].tly, b[ i ].bry );
			for ( unsigned int cwi = b[ i ].tlx; cwi <= b[ i - 1 ].brx; cwi++ ) tilemap[ cwi ][ d ] = TILE_FLOOR;
		}
	}

	// Finally, place monies and the player somewhere safe.
	for ( unsigned int i = 0; i < 16 + 1; i++ )
	{
		unsigned int sx, sy;
		do { sx = rng( 1, 31 ); sy = rng( 1, 31 ); } while ( tilemap[ sx ][ sy ] != TILE_FLOOR );
		if ( i == 16 ) // place player
		{
			px = sx; py = sy;
		}
		else tilemap[ sx ][ sy ] = TILE_MONIES; // place monies
	}
}

// Moves the cursor to ( `x`, `y` ).
void movecursor( unsigned int x, unsigned int y )
{
	print( "\033[" );
	print( uintstr( y + 1 ) );
	print( ";" );
	print( uintstr( x * 2 + 1 ) );
	print( "H" );
}

// Draws the tile `t` at the cursor.
void drawtile( enum tile t )
{
	switch ( t )
	{
		case TILE_FLOOR:  print( "\033[30;100m  \033[0m" ); return;
		case TILE_WALL:   print( "\033[37;107m::\033[0m" ); return;
		case TILE_WATER:  print( "\033[36;104m~~\033[0m" ); return;
		case TILE_LAVA:   print( "\033[93;101m~~\033[0m" ); return;
		case TILE_ACID:   print( "\033[33;42m~~\033[0m" );  return;
		case TILE_SPIKES: print( "\033[97;100m/\\\033[0m" ); return;
		case TILE_HOLE:   print( "\033[30;40m  \033[0m" );  return;

		case TILE_MONIES: print( "\033[93;100m$$\033[0m" ); return;
		
		case TILE_PLAYER: print( "\033[34;106m@@\033[0m" );  return;
	}
}

// Ends the game.
// TODO
#include <stdlib.h>
void /*_Noreturn*/ quit( void )
{
	print( "\033[2J\033[H\033[?25h\n\r" ); // clear screen, move cursor to origin, show cursor
	exit( 0 );
}

// Moves the player, advances the game, and updates the screen.
// Returns whether the move was valid or not; possibly ends the game.
_Bool move( enum move m )
{
	if ( m == MOVE_BAD ) return 0;
	
	// Examine the player's prospective move.
	unsigned int ppx = px, ppy = py;
	if ( m == MOVE_QUIT )       quit();
	else if ( m == MOVE_UP )    ppy--; 
	else if ( m == MOVE_DOWN )  ppy++;
	else if ( m == MOVE_LEFT )  ppx--;
	else if ( m == MOVE_RIGHT ) ppx++;
	else quit();

	switch ( tilemap[ ppx ][ ppy ] )
	{
		case TILE_WALL: return 0; // invalid move, try again
		
		case TILE_WATER: case TILE_LAVA: case TILE_ACID: quit(); // rip

		case TILE_MONIES:
			tilemap[ ppx ][ ppy ] = TILE_FLOOR; // pick up
			
			// Fall through to TILE_FLOOR

		case TILE_FLOOR: // okay move
			
			// Hide existing player.
			movecursor( px, py );
			drawtile( tilemap[ px ][ py ] );
		
			px = ppx; py = ppy;
			
			movecursor( px, py );
			drawtile( TILE_PLAYER );
			return 1;
	}
}

int main( void )
{
	init();
	print( "\033[2J\033[H\033[?25l" ); // clear screen, move cursor to origin, hide cursor
	seed();
	dungen();
	print( "\033[2J\033[H\033[?25l" ); // clear screen, move cursor to origin, hide cursor

	// Draw the initial dungeon.
	for ( int i = 0; i < 32; i++ )
	{
		for ( int j = 0; j < 32; j++ ) drawtile( tilemap[ j ][ i ] );
		print( "\033[0m\n" );
#ifdef NLCR
		print( "\r" );
#endif
	}
	
	// Draw the initial player.
	movecursor( px, py );
	drawtile( TILE_PLAYER );	

	for ( ;; )
	{
		move( getmove() );
	}
}

