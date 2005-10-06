int debugging = 0;

#define DEBUG(x) do {if (debugging) x;} while (0)

// file formats
#define ASCII 0
#define BINARY 1

// data ordering
#define ROW_MAJOR 0
#define COLUMN_MAJOR 1

// approximate values of window manager borders & desktop borders (stay out of these)
#ifdef __APPLE__
 int top_frame=30, bottom_frame=0, left_frame=0, right_frame=2;
 int top_safe = 1, bottom_safe=5, left_safe=5, right_safe=1;
#else // __APPLE__
 int top_frame=20, bottom_frame=5, left_frame=4, right_frame=5;
 int top_safe = 1, bottom_safe=10, left_safe=10, right_safe=1;
#endif // __APPLE__

#ifdef __APPLE__
#define FAST_APPLE_VERTEX_EXTENSIONS
#endif // __APPLE__
