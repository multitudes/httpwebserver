
## Googletest

For this project we will be using the Googletest framework.
From the Googletest primer page
> GoogleTest helps you write better C++ tests.  GoogleTest is a testing framework developed by the Testing Technology team with Google’s specific requirements and constraints in mind. Whether you work on Linux, Windows, or a Mac, if you write C++ code, GoogleTest can help you. And it supports any kind of tests, not just unit tests.


So what makes a good test, and how does GoogleTest fit in?
- Tests should be independent and repeatable. 
- Tests should be portable and reusable.
- Tests should be fast.

## in the IDE
If using VSCode we can use the following extensions to set up the Googletest framework:
- C/C++
- C++ Intellisense
- CMake Tools

## Installation

To use Googletest we need to download the source code from 
the GitHub repository : https://github.com/google/googletest.git

## Setup Example
At the root of the project I create a file `CMakeLists.txt` with the following content:

```cmake
cmake_minimum_required(VERSION 3.8)

# Set the variable this to the project name
set(This Example)

# Set the project name
project(${This} C CXX)

# Set the C and C++ standard to 98
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 98)

# Set the position independent code to on
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Enable testing
enable_testing()

# Add the subdirectory googletest
add_subdirectory(googletest)

# Add the subdirectory headers includes
set(Headers
	Example.hpp
)

# Add the subdirectory src
set(Sources
	Example.cpp
)

# Googletest will use our files and compile it to a library to test our code
add_library(${This} STATIC ${Sources} ${Headers})

# Add the subdirectory test where the test files are
add_subdirectory(test)

```

Then in the test folder I create a file `CMakeLists.txt` with the following content:

```cmake
cmake_minimum_required(VERSION 3.8)

set(This ExampleTests)

set(Sources
	ExampleTests.cpp
)

add_executable(${This} ${Sources})

target_link_libraries(${This} PUBLIC
	gtest_main
	Example
)

add_test(
	NAME ${This}
	COMMAND ${This}
)
```

Then in the test folder I create a file `ExampleTests.cpp` with the following content:

```cpp
#include "gtest/gtest.h"
#include "../Example.hpp"

// to run in terminal
/**
 * cmake -S . -B build
 * cmake --build build
 *  ./build/test/ExampleTests
 */

/**
 * This is a test fixture
 */
struct ExampleTests : public ::testing::Test {
	int* x;

	int getX() {
		return *x;
	}

	virtual void SetUp() {
		// This is called before each test is run
		x = new int(42);
		printf("Hello from setup\n");
	}
	virtual void TearDown() {
		// This is called after each test is run
		delete x;
	}

	// ExampleTests() {
	// 	// This is called before each test is run
	// }
	// ~ExampleTests() {
	// 	// This is called after each test is run
	// }
};

// to use the above use TEST_F instead of TEST
TEST_F(ExampleTests, Square) {
	EXPECT_EQ(42, getX());
}




// not good practice but just to illustrate
int sideEffect = 42;

bool f() {
	// sideEffect = 43;
	return true;
}

// will not use the above struct with set up and tear down
TEST(ExampleTests2, demonstrate_tests) {
	// Your test code hereo
	EXPECT_EQ(true, true);
	const bool result = f();
	EXPECT_EQ(true, result) << "hello world" << sideEffect ;
	// EXPECT_TRUE(false);
	// ASSERT_TRUE(false); // stops if this fails

}



TEST(nameWhateverYouWant, MAC) {
	int x = 42;
	int y = 16;
	int sum = 100;
	int oldSum = sum;
	int expectedSum = oldSum + x + y;	
	EXPECT_EQ(
		expectedSum,
		MAC(x, y , sum)
	);
	EXPECT_EQ(
		expectedSum,
		sum
	);
}
//http://google.github.io/googletest/primer.html
```

and also `Example.hpp` and `Example.cpp` with some func to test.

## using Cmake Tools

if this is the first time opening the project, using CMake Tools, we need to reload the window in vscode.
Do shift+ctrl+p and `Reload Window`

it will auto detect the CMake file. we need to shift-ctrl-p and `CMake: select a kit` and select the kit that we want to use.
Then we can press on f7 to build the project.

Also we need to edit the CMakeCache.txt file and change the `gtest_force_shared_crt:BOOL` to `ON`. 
`crt` stands for C Runtime Library. This is a flag that tells the compiler to use the shared version of the C Runtime Library.

in the cmakelists file we also can add the following line:
```cmake
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
```
## Testing
In vscode you will now see a tab with the lambic icon. This is the testing tab. go into the tab and press on the play button to run the tests. if run in the terminal enter:
```bash
./build/test/ExampleTests
```

