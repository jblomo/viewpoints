int debugging = 0;

#define DEBUG(x) do {if (debugging) x;} while (0)

// file formats
#define BINARY 1
#define ASCII 0

// approximate values of window manager borders & desktop borders (stay out of these)
#ifdef __APPLE__
const int top_frame=25, bottom_frame=1, left_frame=1, right_frame=2;
const int top_safe = 1, bottom_safe=5, left_safe=5, right_safe=1;
#else // __APPLE__
const int top_frame=20, bottom_frame=5, left_frame=4, right_frame=5;
const int top_safe = 1, bottom_safe=10, left_safe=10, right_safe=1;
#endif // __APPLE__

#ifdef __APPLE__
#define FAST_APPLE_VERTEX_EXTENSIONS
#endif // __APPLE__
