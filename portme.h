// Sets up any platform-specific facilities.
void init( void );

// Prints the null-terminated ASCII string `s` to a TTY somewhere.
void print( const char * s );

// Seeds the PRNG.
void seed();

// Blocks until a button is pressed down somewhere, then returns that movement.
enum move getmove( void );



