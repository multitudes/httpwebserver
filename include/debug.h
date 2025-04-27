#ifndef DEBUG_H
# define DEBUG_H

# include <cstdio>

// #define NDEBUG 1

/*
This allows us to use the debug macro to print debug messages but to 
compile them out when NDEBUG is defined.
If we define NDEBUG in the makefile or as a flag -DNDEBUG, 
the debug macro will be replaced with an empty macro.
during submission we will remove the debug macro from the code
including the macro below which is not allowed by norminette.
*/

// Define color codes
#define BLACK   "\033[0;30m"
#define RED     "\033[0;31m"
#define GREEN   "\033[0;32m"
#define YELLOW  "\033[0;33m"
#define BLUE    "\033[0;34m"
#define MAGENTA "\033[0;35m"
#define CYAN    "\033[0;36m"
#define WHITE   "\033[0;37m"
#define PASTEL_RED "\033[0;91m"
#define PASTEL_GREEN "\033[0;92m"
#define PASTEL_YELLOW "\033[0;93m"
#define PASTEL_BLUE "\033[0;94m"
#define PASTEL_MAGENTA "\033[0;95m"
#define PASTEL_CYAN "\033[0;96m"
#define RESET   "\033[0m"

# ifdef NDEBUG
# define debug(M, ...)
// # define debugcolor(C, M, ...)
// # define debuglog(C, M, ...)
# else
# define debug(M, ...) ::fprintf(stderr, "\033[0;92mDEBUG: %s:%d: " M "\033[0m\n", \
        __FILE__, __LINE__, ##__VA_ARGS__)
# endif

# define debugcolor(C, M, ...) ::fprintf(stderr, "%sDEBUG %s:%s:%d: " M "\033[0m\n",\
        C, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
# define debuglog(C, M, ...) ::fprintf(stderr, "%s[Server] : " M "\033[0m\n",\
                C, ##__VA_ARGS__)
#endif //DEBUG_H
