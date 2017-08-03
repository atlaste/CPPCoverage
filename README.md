# Free C++ Code Coverage

CPPCoverage is a Visual Studio extension that calculates code coverage and profile data for C++ applications and Visual Studio C++ native tests. Basically it provides 
you with an easy-to-use, light-weight C++ code coverage and profiler, right from Visual Studio and with the features you expect from tools like these. 

# Installation

Prerequisites: VS2013 or VS2015. Any edition should work, even though this is only tested on VS2015 Community.

- Install this extension: https://github.com/atlaste/VSOpenCPPCoverage/raw/master/CoverageExt/CoverageExt.vsix .

# Getting started

Working with CPPCoverage is a breeze. Basically install and use, there's nothing more to it:

- Either create a standard C++ / MS Test application, or run a simple C++ / console application. Note that OpenCppCoverage assumes that there is no user input during the test run.
- Build your solution in Debug mode. Make sure to compile with '/DEBUG'; '/DEBUG:FastLink' is currently unsupported .
- Right-click in solution explorer on the test or application project, click "Run code coverage".
- Open a file that you want to show coverage info for. If the file is already open, close it and open it again. 
- For an overview, go to Tools -> Coverage report
- Coverage is measured in a background thread. Watch the output window to see when it's finished.

# Features

- Highlighting covered and uncovered code directly from Visual Studio:

![alt tag](Screenshots/Highlighting.png)

- Report generation of covered code (Tools -> Coverage report):

![alt tag](Screenshots/CoverageReport.png)

- Run code coverage on VS unit tests (Right click VS project -> Run code coverage):

![alt tag](Screenshots/SolutionExplorer.png)

- Hide code from code coverage results using #pragma DisableCodeCoverage and #pragma EnableCodeCoverage:

![alt tag](Screenshots/Pragmas.png)

- Sampling-based profiling data is gathered as a by-product

![alt tag](Screenshots/ProfileData.png)

- IsDebuggerAttached is overwritten with 'return false' so that you can distinguish between debugger and test sessions. 
- The CodeCoverage output window will give you the current status of the process. Stdout is forwarded here as well.
- Small memory footprint and very fast. Even if you have a million lines of code, CPPCoverage will only use kilobytes of memory for coverage.

# Experimental Features

- Static code analysis can be enabled, to get rid of if-then-else quirks. 
- In your test code, you can tell the coverage tool to ignore files and folders. So, everything is handled from code; no pesky configuration files need to be managed. 

For an overview of these [runtime notifications](notifications.md).

# Support and maintenance 

CPPCoverage is 100% open source and 100% for free.

At NubiloSoft, we use this CPPCoverage addin on a daily basis in a real environment on a huge C++ code base. We want this to be good. And since this is not our core 
business, we are willing to make it available for free. Simple as that. 

Over the period that we've used this extension, it has proven to be both reliable and stable, resulting in only a very few issues and very little work for us to maintain it. 
However, if you do find a bug or think the tool lacks a feature, please let us know by posting it in 'issues'. 

# Profile data

Our profile data is gathered in a very simple way: every 5 milliseconds we simply force a context switch, after which we gather the stack trace. For the entire stack trace, we then 
update the number of times that line has been hit (both for the top-most frame and for all frames). We only consider code that is available; performance is *not* measured for 
third-party libraries, even if the PDB is available. 

This process will give us two numbers, that are shown in Visual Studio: 

1. The percentage of time the method spends on this line of code. Note that if this is a function call, the time spent could be very high.
2. The percentage of time the application spends on this specific line of code. This is excluding function calls, so most lines will have very low numbers.

For those familiar with the tools, this is pretty similar to what tools like Sleepy and Very Sleepy do.

As an example, if you spot "50%/20%" after a line of code, this basically means that 50% of the time is spent on this line of code (or in one of the child function calls) and 
20% of the time is spent on this line of code (in total). 

If profile information is insignificant (e.g. 0%), we simply won't show it. You might ask yourself: why 5 milliseconds? Well, simply put, context switches have a serious impact on 
performance, and this appears to be the sweatspot where the tool doesn't have significant impact on the performance of your test.

# If the tool doesn't work...

There are a few likely suspects to check:

- Check the Coverage output window in Visual Studio!
- Have you loaded a solution?
- Are PDB's available? Can you debug the application? If not, code coverage won't work.
- If your (Native unit test DLL) unit tests aren't running in Visual Studio, they're also not running in the coverage tool. Obviously this is the same when you're using an executable.
- Is all your source code in a child folder of your solution? If not, you can run Coverage-x86 or Coverage-x64 as executable and store the output in the solution folder. 
Basically the recipe is as follows:

[install folder]\Resources\Coverage[-x86 / -x64].exe -o "[solution folder]\CodeCoverageTest.cov" -p "[root path of your code; omit the trailing '\']" -- "[executable to run...]" [... optional arguments...]

# Advanced users

Most people want to integrate code coverage in their test environment. Coverage-x86.exe and Coverage-x64.exe will provide just that. We support emitting Cobertura XML files, 
which can be processed by a lot of tools. 

If you want even more control, grab the code from the Coverage project, change the executable that's executed and you're done. 

# Measuring code coverage

In a nutshell, our test tool measures everything by creating a debugger process and putting breakpoints on every line of code. After a breakpoint has been hit, the breakpoint 
(assembler) instruction is removed from the program again. This breakpoint process has only a tiny bit of overhead, because it basically relies on a single context switch 
per breakpoint. Also, this means that the amount of memory that you need is roughly bound by the number of lines in your application. 

In the early versions of this tool, we've used OpenCPPCoverage as an external dependency. And even though OpenCPPCoverage is pretty decent, we feel it just cannot deliver 
everything that we need. Therefore we've been working on a better alternative, which is available *now*:

- OpenCPPCoverage seems to be pretty heavy. We wanted something that's fast as hell, since we're measuring all the time. 
- The relevant #pragma's are handled by the coverage tool, so that files are properly ignored by the coverage engine.
- We're working with production-grade software, so we want the memory footprint of out coverage tool to be very small. 
- We want integration in our test suites. It makes little sense to run a separate memory leak checker, a separate coverage checker and a separate unit test runner. We want all 
  those things at once.
- Better handling of filters for "normal" solutions. We want to measure coverage on our solution, nothing more and nothing less.
- IsDebuggerAttached is giving us a hard time. We've implemented a nice workaround for this in the new coverage tool.
- We found that a lot of time was spent writing Cobertura files. Even though the new coverage tools support Cobertura XML files, it's not preferred. We've created a new 
  text-based file, which is way smaller and more suitable for code coverage. 

The projects Coverage-x86 and Coverage-x64 are the result of this, which provide this new coverage tool. For the most part it already works great; for 
most applications you don't even notice that coverage is being measured. 

# Templates, templates...

If you have templated code, code coverage will be measured for any of the templates. This was not an easy decision, after all: if you have a different template parameter, 
you actually end up with different code. Still, if the specialization for type A and type B will run the same code, we find it unlikely that the quality of your tests is 
affected (just because you test A and not B). In most (but not all!) cases, we believe it's the coverage of the members of A and B that you call that tell the story.

In fact, our Coverage engine measures this and emits 'p' for 'partial coverage' in the results. 

So... in short: if a template specialization is hit, we'll mark the line as green, regardless if all possible template arguments are tested or not.

# License

Your friendly BSD license. Please give credits where credits are deserved.
