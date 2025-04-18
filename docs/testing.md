# Testing

For this project in C++ we will be using Googletest as the main testing framework.

For some debugging it is also nice to use netcat so I will also show how to use netcat in the project.

# Netcat

The headers are ending always with this sequence of characters: "\r\n\r\n". This is the end of the headers and the beginning of the body if present. Usually a GET request does not have a body. The body is used in POST requests but then the content length header is used to specify the length of the body.
So a header without content length would not have a body and if anything comes after the headers will be discarded. Get requests dont have a body. 

I can connect to my server's port 4243 using netcat. Just by typing 
```bash
nc localhost 4243
```

but to send the headers with the CRLF sequence I cannot just type the request in the terminal. It is much easier to pipe the request to the netcat command. 

```bash
echo -e "GET / HTTP/1.1\r\nHost: localhost\r\n\r\n" | nc localhost 4243
```

the option `-e` is to enable interpretation of backslash escapes. This is needed to interpret the `\r` and `\n` as carriage return and newline characters. Also I can use the `-n` option with echo to avoid the newline at the end of the string. 

sending this command my server will respond with the headers and the body of the response.

In the same way I can send malformed requests and see how the server responds. 
```bash
echo -e "\r\n\r\na" | nc localhost 4243
```

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



## github workflows

To use github workflows we need to create a folder `.github/workflows` and create a file `main.yml` with the following content:

```yml
# .github/workflows/ci.yml
name: CI

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest

    steps:
    - name: Checkout code
      uses: actions/checkout@v2

    - name: Configure CMake
      run: cmake -S . -B build

    - name: Build
      run: cmake --build build

    - name: Run tests
      run: ctest --test-dir build# .github/workflows/ci.yml
name: CI

on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest

    steps:
    - name: Checkout code
      uses: actions/checkout@v2
      with:
        submodules: true  

    - name: Configure CMake
      run: cmake -S . -B build

    - name: Build
      run: cmake --build build

    - name: Run tests
      run: ctest --test-dir build

```

This will run the tests on every push and pull request.
This workflow will:
- Checkout your code: Uses the actions/checkout@v2 action to clone your repository.
- Set up CMake: Uses the lukka/get-cmake@v3 action to install CMake.
- Configure CMake: Runs cmake -S . -B build to configure the project.
- Build the project: Runs cmake --build build to build the project.
- Run the tests: Runs ctest --test-dir build to execute the tests.
- Commit and push the workflow file:

```bash
git add .github/workflows/ci.yml
git commit -m "Add GitHub Actions workflow for CI"
git push origin main
```

once the workflow is pushed to the repository, go to the actions tab in github and you will see the workflow running.


## on the mac the vscode debugger is LLDB

Okay, here are the equivalent `curl` commands for your terminal testing, along with ways to stop a hanging debugger in VS Code on macOS.

### `curl` Command Equivalents

Remember to run your C++ web server so it's listening on `localhost:4244` before executing these commands. The `-v` flag is added for verbose output, which helps see the request headers sent and the response headers received.

1.  **POST with multipart/form-data**

    * First, create the dummy file content:
        ```bash
        echo -e "This is the content of the file.\nSecond line." > report.txt 
        ```
    * Then, run `curl`:
        ```bash
        curl -v -X POST \
             -F "text_field=Simple text value" \
             -F "file_upload=@report.txt;type=text/plain" \
             http://localhost:4244/cgi/hello.py
        ```
    * Clean up the dummy file:
        ```bash
        rm report.txt
        ```
    * **Explanation:**
        * `-X POST`: Specifies the POST method (though `-F` implies POST).
        * `-F "field=value"`: Sends a simple form field.
        * `-F "name=@filename;type=mimetype"`: Sends a file. `@report.txt` tells curl to read the content from `report.txt`. `;type=text/plain` explicitly sets the Content-Type for this part of the multipart message.
        * `curl` automatically generates the `Content-Type: multipart/form-data; boundary=...` header and formats the request body.

2.  **POST with Chunked Transfer Encoding**

    ```bash
    # Pipe the data to curl using echo -n (to avoid extra newline) and -e (to interpret escapes like \n)
    echo -n -e "This is the first chunk.\nAnd this is the second chunk.\nFinally, the third chunk." | \
    curl -v -X POST \
         -H "Content-Type: text/plain" \
         -H "Transfer-Encoding: chunked" \
         --data-binary @- \
         http://localhost:4244/cgi/hello.py
    ```
    * **Explanation:**
        * `-X POST`: Specifies the POST method.
        * `echo -n -e "..." |`: Pipes the raw data string to curl's standard input. `-n` prevents echo adding a trailing newline, `-e` enables interpretation of backslash escapes.
        * `-H "Content-Type: text/plain"`: Sets the content type header.
        * `-H "Transfer-Encoding: chunked"`: Explicitly tells curl (and the server) that you intend to send chunked data. When combined with `--data-binary @-`, curl will correctly format it.
        * `--data-binary @-`: Tells curl to read raw binary data from standard input (`-`). This combination often triggers chunked encoding automatically in curl for HTTP/1.1, but the explicit header makes the intent clear for testing.

3.  **DELETE Request**

    ```bash
    curl -v -X DELETE http://localhost:4244/cgi/hello.py/resource/to/delete
    ```
    * **Explanation:**
        * `-X DELETE`: Specifies the DELETE method.
        * The URL includes the path info `/resource/to/delete`.
        * No data is sent with this request.

### Stopping a Hanging Debugger in VS Code on macOS

When the VS Code debugger hangs (the UI might become unresponsive, the stop button doesn't work, or the debugged process just sits there), here are several ways to stop it, from least to most disruptive:

1.  **Use the VS Code Debug Toolbar/Command Palette:**
    * Try clicking the red square **Stop** button on the floating debug toolbar again.
    * Open the Command Palette (`Cmd + Shift + P`), type "Debug: Stop", and press Enter. Sometimes this works even if the button is unresponsive.

2.  **Restart the Debug Session:**
    * Try the **Restart** button (the curved arrow) on the debug toolbar. This might detach the faulty session and start a new one.

3.  **Close the Project Window:**
    * Simply closing the VS Code window (`Cmd + W`) where the debugger is running will usually terminate the debug session. Reopen the project afterwards.

4.  **Use Activity Monitor:**
    * Open **Activity Monitor** (Applications -> Utilities -> Activity Monitor).
    * In the search bar, look for processes related to your debug session. Common names include:
        * `lldb` (the default C++ debugger on macOS) or `lldb-mi`
        * `gdb` (if you configured VS Code to use it)
        * `debugserver` (a process lldb might use)
        * The name of your **executable** (your webserver). It might be stuck.
    * Select the suspect process (start with `lldb` or your executable).
    * Click the **"X" button** in the Activity Monitor toolbar.
    * Choose **"Quit"**. If that doesn't work after a few seconds, click "X" again and choose **"Force Quit"**. Be cautious with Force Quit.

5.  **Use the Terminal:**
    * Open a new Terminal window.
    * Find the Process ID (PID) of the debugger or your server process. Use `ps aux | grep <name>`, replacing `<name>` with `lldb`, `gdb`, or the name of your executable.
        ```bash
        ps aux | grep lldb 
        ps aux | grep your_server_executable_name 
        ```
    * Look for the relevant process and note its PID (the second column).
    * Try to terminate it gracefully: `kill <PID>`
    * If it doesn't stop after a few seconds, force kill it: `kill -9 <PID>` (Use `-9` as a last resort, as it doesn't allow the process to clean up).

6.  **Force Quit VS Code:**
    * If VS Code itself is completely frozen, press `Option + Command + Esc` to open the "Force Quit Applications" window.
    * Select "Visual Studio Code".
    * Click **"Force Quit"**. This will kill VS Code and any child processes, including the debugger.

Usually, starting with the VS Code controls and escalating to Activity Monitor or `kill` if necessary will resolve the issue.