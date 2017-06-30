# Runtime notifications

Runtime notifications are an experimental feature of CPPCoverage.

Basically you can teach your program to communicate with CPPCoverage, thereby setting options at runtime. 

What you need is a small stub in your program, which will act as a hook to the coverage tool:

```C++
extern "C" { __declspec(noinline) static void __stdcall PassToCPPCoverage(size_t count, const char* data) { __nop(); } }
```

It doesn't matter if you put the code in any of your DLL's or in an executable. Also, you can call the method as many times as you want.

Note: The 'nop' is just there to ensure there are at least two instructions in the method. If you don't like it (because reasons...), you can replace it with whatever else you want.

I recommend using std::string to pass options, but if you like C-style strings that's obviously okay as well.

# Available options

**IGNORE FOLDER: &lt;&lt;folder name&gt;&gt;**

Excludes a whole physical folder from the coverage process. You are allowed to use relative paths (usually to the solution folder, *not* the executable folder).

**IGNORE FILE: &lt;&lt;file name&gt;&gt;**

Excludes a physical file from the coverage process. You are allowed to use relative paths (usually to the solution folder, *not* the executable folder).

**ENABLE CODE ANALYSIS**

Enables *all DLL's that are loaded from this point in the program* for code analysis. By default, code analysis is disabled.

**DISABLE CODE ANALYSIS**

Disables *all DLL's that are loaded from this point in the program* for code analysis. By default, code analysis is disabled.

# Example

```C++
std::string opts = "IGNORE FOLDER: MinimumTestApp";
PassToCPPCoverage(opts.size(), opts.data());
```

