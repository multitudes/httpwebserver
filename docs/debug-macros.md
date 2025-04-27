About the debug.h file:
how would i change this to make it more c++ 98 

Yes, using macros (`#define`) and calling C standard library functions like `fprintf` (using `::fprintf` or just `fprintf`) is perfectly fine and standard practice in C++98.

1.  **Macros (`#define`)**: The C preprocessor, including `#define`, is part of the C++ standard, including C++98. While C++ often provides better alternatives (like `const` variables, `enum`, `inline` functions, templates), macros are still valid, especially for conditional compilation (`#ifdef`) and simple logging/debugging utilities as you have shown.

2.  **`fprintf`**: This is a standard C library function declared in `<stdio.h>` (or `<cstdio>` in C++). C++98 includes the C standard library, so `fprintf` is available and commonly used.

3.  **`::fprintf`**: The `::` operator explicitly accesses the global namespace. This is valid C++98 syntax. It's often used to disambiguate when calling C functions from within C++ namespaces or classes, ensuring you get the global C library function. It's perfectly acceptable.

**However, one detail:**

*   **Variadic Macros (`##__VA_ARGS__`)**: The specific syntax `##__VA_ARGS__` for handling variable arguments in macros (especially the `##` part to swallow the preceding comma if `__VA_ARGS__` is empty) is technically a feature introduced in **C99** and standardized in **C++11**. While many C++98 compilers (like GCC and Clang) supported it as an **extension**, it's not strictly part of the C++98 standard itself.

So, while macros and `::fprintf` are C++98, the exact variadic macro syntax you're using relies on features formally standardized later or on compiler extensions. It will likely *work* with common C++98 compilers, but isn't guaranteed by the C++98 standard alone.

Okay, replacing C-style `fprintf` and variadic macros entirely while keeping similar functionality in pure C++98 is tricky, primarily because C++98 lacks built-in support for variadic templates or easy `printf`-like formatting with `iostream`.

The most idiomatic C++ approach involves creating a stream-like object. However, this **changes how you use the debug macros**. Instead of a format string and arguments, you'll use the `<<` operator like `std::cerr`.

Here's a possible implementation:

```cpp
#ifndef DEBUG_H
#define DEBUG_H

#include <iostream> // Use iostream for C++ output
#include <string>   // For std::string if needed
#include <ctime>    // For potential timestamping if desired (optional)

// Define color codes (macros are fine for constants)
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

#ifdef NDEBUG // If NDEBUG is defined, provide dummy implementations

// A dummy class whose operator<< does nothing
class NullStream {
public:
    template <typename T>
    NullStream& operator<<(const T& /*value*/) {
        return *this;
    }
    // Handle manipulators like std::endl
    typedef std::ostream& (*Manipulator)(std::ostream&);
    NullStream& operator<<(Manipulator /*manip*/) {
        return *this;
    }
};

// Define macros to create instances of the dummy class
#define debug NullStream()
#define debugcolor(C) NullStream()
#define debuglog(C) NullStream()

#else // NDEBUG is not defined, provide real implementations

// A helper class to manage the output stream for debugging
class DebugStream {
private:
    bool add_newline; // Flag to add newline in destructor

public:
    // Constructor for debug() and debugcolor()
    DebugStream(const char* color, const char* file, const char* func, int line)
        : add_newline(true) {
        std::cerr << color << "DEBUG " << file << ":" << func << ":" << line << ": ";
    }

    // Constructor for debuglog()
    DebugStream(const char* color) : add_newline(true) {
        std::cerr << color << "[Server] : ";
    }

    // Destructor adds newline and resets color
    ~DebugStream() {
        if (add_newline) {
            std::cerr << RESET << std::endl; // std::endl also flushes
        } else {
            std::cerr << RESET; // Reset color without newline if needed
        }
    }

    // Overload operator<< for various types
    template <typename T>
    DebugStream& operator<<(const T& value) {
        std::cerr << value;
        return *this;
    }

    // Handle manipulators like std::endl
    // If std::endl is used, we don't want the destructor to add another newline
    typedef std::ostream& (*Manipulator)(std::ostream&);
    DebugStream& operator<<(Manipulator manip) {
        // Check if the manipulator is std::endl
        if (manip == static_cast<Manipulator>(std::endl)) {
            add_newline = false; // Prevent destructor from adding another newline
        }
        manip(std::cerr); // Apply the manipulator
        return *this;
    }
};

// Define macros to create temporary instances of DebugStream
// Note: __FUNCTION__ might not be strictly C++98, but widely supported.
// If not available, you might have to remove it.
#define debug DebugStream(PASTEL_GREEN, __FILE__, __FUNCTION__, __LINE__)
#define debugcolor(C) DebugStream(C, __FILE__, __FUNCTION__, __LINE__)
#define debuglog(C) DebugStream(C)

#endif // NDEBUG

#endif // DEBUG_H
```

**How Usage Changes:**

Your existing debug calls need to be modified:

```cpp
// Old C-style:
// debug("Processing request for fd %d", conn.client_fd);
// debuglog(RED, "Error: Failed to open file '%s'", filename);
// debugcolor(CYAN, "Value is %d", some_value);

// New C++ stream-style:
debug << "Processing request for fd " << conn.client_fd;
debuglog(RED) << "Error: Failed to open file '" << filename << "'";
debugcolor(CYAN) << "Value is " << some_value;
```

**Explanation:**

1.  **Includes:** Uses `<iostream>` for `std::cerr`.
2.  **`NullStream`:** When `NDEBUG` is defined, the macros create a temporary `NullStream` object. Its `operator<<` does nothing, effectively compiling out the debug messages with minimal overhead.
3.  **`DebugStream`:** When `NDEBUG` is *not* defined:
    *   The macros create a temporary `DebugStream` object.
    *   The constructor immediately prints the prefix (color, file/line info, etc.) to `std::cerr`.
    *   Subsequent `<<` operations append data to `std::cerr`.
    *   The destructor (`~DebugStream`) is called automatically when the temporary object goes out of scope (at the end of the full expression, usually the semicolon). It prints the `RESET` color code and a newline (`std::endl`).
    *   It uses templates for `operator<<` to handle different data types, which is valid C++98.
    *   It includes special handling for `std::endl` to avoid double newlines.
4.  **Macros:** The macros are now simpler, just creating instances of the appropriate class (`DebugStream` or `NullStream`).
5.  **`__FUNCTION__`:** Note that `__FUNCTION__` is technically C99/C++11, but widely supported as an extension in C++98 compilers. If your compiler strictly enforces C++98 and doesn't support it, you might need to remove it from the macros and constructors.

This approach uses C++ streams and avoids C `stdio` functions and non-standard variadic macros, but requires adapting your logging calls to the `<<` stream syntax.

Similar code found with 1 license type