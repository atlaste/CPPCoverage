#pragma once

#include "FileCallbackInfo.h"
#include "RuntimeOptions.h"
#include "RuntimeNotifications.h"
#include "CallbackInfo.h"
#include "ProfileNode.h"
#include "Util.h"

#include "Disassembler/ReachabilityAnalysis.h"

#include <algorithm>
#include <format>
#include <iostream>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <Windows.h>
#include <psapi.h>

#pragma warning(disable : 4091)
#include <DbgHelp.h>
#pragma warning(default : 4091)

struct CoverageRunner
{
  CoverageRunner(RuntimeOptions& opts) : options(opts),
    notifications(opts),
    debugInfoAvailable(false),
    debuggerPresentPatched(false),
    coverageContext(opts),
    profileInfo()
  {}

  static BOOL CALLBACK SymEnumLinesCallback(PSRCCODEINFO lineInfo, PVOID userContext)
  {
    CallbackInfo* info = reinterpret_cast<CallbackInfo*>(userContext);

    if (info->fileInfo->PathMatches(lineInfo->FileName))
    {
      // Try to find if file exists (and can be covered)
      auto file = std::string(lineInfo->FileName);
      if (!std::filesystem::exists(file))
      {
#ifndef NDEBUG
        if (RuntimeOptionsSingleton::Instance().isAtLeastLevel(VerboseLevel::Error))
        {
          std::cerr << std::format("Impossible to find file : {0}", file) << std::endl;
        }
#endif
        return FALSE;
      }

      PVOID addr = reinterpret_cast<PVOID>(lineInfo->Address);
      auto it = info->breakpointsToSet.find(addr);
      if (it == info->breakpointsToSet.end())
      {
        // Find line info
        auto fileLineInfo = info->fileInfo->LineInfo(file, lineInfo->LineNumber);
        if (fileLineInfo)
        {
          // Only create breakpoint if we haven't already.
          if (info->breakpointsToSet.insert(addr).second)
          {
            if (info->registerLines)
            {
              fileLineInfo->DebugCount++;
            }

            info->processInfo->breakPoints[addr] = BreakpointData('\0', fileLineInfo);
          }
        }
      }
    }

    return TRUE;
  }

  static BOOL CALLBACK SymEnumSymbolsCallback(PSYMBOL_INFO symInfo, ULONG symbolSize, PVOID userContext)
  {
    if (symbolSize != 0)
    {
      CallbackInfo* info = reinterpret_cast<CallbackInfo*>(userContext);
      ReachabilityAnalysis ra(info->processInfo->Handle, DWORD64(symInfo->Address), SIZE_T(symInfo->Size));
      info->reachableCode.emplace_back(std::move(ra));
    }
    return TRUE;
  }

  const RuntimeOptions& options;
  RuntimeNotifications notifications;
  bool debugInfoAvailable;
  bool debuggerPresentPatched;
  std::unordered_set<std::string> loadedFiles;
  FileCallbackInfo coverageContext;

  std::vector<std::tuple<PVOID, BYTE, PVOID, BYTE>> passToCoverageMethods;

  std::unordered_map<std::string, std::unique_ptr<ProfileFrame>> profileInfo;

  // PIDs of child processes we must detach from after the current debug event
  // is continued (because they have a bitness that is incompatible with ours).
  std::vector<DWORD> pendingDetach;

  // Handles of sibling-bitness CPPCoverage.exe helper processes we launched
  // to instrument cross-bitness child processes. We wait on these at the end
  // of the debug loop before writing our own report.
  std::vector<HANDLE> siblingCoverageProcesses;

  // Output files produced by sibling-bitness CPPCoverage.exe helper
  // processes. Main.cpp reads this after Start() returns so it can fold
  // them into the merged coverage file.
  std::vector<std::string> auxiliaryCoverageOutputs;

  // Returns true if the given process has the same bitness as ourselves and
  // can therefore be safely debugged. A 64-bit debugger cannot reliably drive
  // a 32-bit child (WOW64) process: symbol loading and cross-bitness
  // breakpoint writes corrupt the target. When the user launches something
  // like vstest.console.exe (x64), it may in turn spawn x86 workers such as
  // testhost.x86.exe; those must be detached rather than instrumented.
  bool IsProcessCompatibleBitness(HANDLE hProcess)
  {
    USHORT ourMachine;
#if defined(_M_AMD64)
    ourMachine = IMAGE_FILE_MACHINE_AMD64;
#elif defined(_M_IX86)
    ourMachine = IMAGE_FILE_MACHINE_I386;
#elif defined(_M_ARM64)
    ourMachine = IMAGE_FILE_MACHINE_ARM64;
#else
    ourMachine = IMAGE_FILE_MACHINE_UNKNOWN;
#endif

    // Prefer IsWow64Process2 because it reports the real machine type, and
    // unlike IsWow64Process it works correctly when a 64-bit debugger
    // inspects a same-bitness child (see the old TODO FIXME block further
    // down in this file).
    typedef BOOL(WINAPI* pfnIsWow64Process2)(HANDLE, USHORT*, USHORT*);
    static auto fnIsWow64Process2 = reinterpret_cast<pfnIsWow64Process2>(
      GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process2"));

    if (fnIsWow64Process2)
    {
      USHORT processMachine = 0;
      USHORT nativeMachine = 0;
      if (fnIsWow64Process2(hProcess, &processMachine, &nativeMachine))
      {
        // processMachine is IMAGE_FILE_MACHINE_UNKNOWN when the child
        // is a native process (not running under WOW64). In that case
        // the actual machine equals the native OS machine.
        USHORT actualMachine = (processMachine == IMAGE_FILE_MACHINE_UNKNOWN)
          ? nativeMachine
          : processMachine;
        return actualMachine == ourMachine;
      }
    }

    // Fallback for very old Windows: use IsWow64Process. This can only
    // answer the question for a 64-bit debugger; a 32-bit debugger on a
    // 64-bit OS cannot distinguish a native 64-bit child from itself via
    // this API, so we conservatively report "compatible" and let the
    // existing failure modes apply.
    BOOL isWow64 = FALSE;
    if (IsWow64Process(hProcess, &isWow64))
    {
#if defined(_WIN64)
      return isWow64 == FALSE;
#else
      return true;
#endif
    }

    return true;
  }

  // Quote a single command-line argument the way CommandLineToArgvW expects
  // it back. Only escapes what's strictly necessary (backslashes preceding a
  // quote, plus the surrounding quote when the arg contains spaces, tabs,
  // or quotes).
  static std::string QuoteArg(const std::string& arg)
  {
    bool needsQuote = arg.empty() ||
      arg.find(' ') != std::string::npos ||
      arg.find('\t') != std::string::npos ||
      arg.find('"') != std::string::npos;

    if (!needsQuote)
    {
      return arg;
    }

    std::string out;
    out.reserve(arg.size() + 2);
    out.push_back('"');
    for (size_t i = 0; i < arg.size(); ++i)
    {
      size_t backslashes = 0;
      while (i < arg.size() && arg[i] == '\\')
      {
        ++backslashes;
        ++i;
      }

      if (i == arg.size())
      {
        out.append(backslashes * 2, '\\');
        break;
      }

      if (arg[i] == '"')
      {
        out.append(backslashes * 2 + 1, '\\');
        out.push_back('"');
      }
      else
      {
        out.append(backslashes, '\\');
        out.push_back(arg[i]);
      }
    }
    out.push_back('"');
    return out;
  }

  // Return the full path to the sibling-bitness CPPCoverage executable, or
  // empty string when none can be found next to us. We assume the standard
  // naming scheme used by this project: "Coverage-x64.exe", "Coverage-x86.exe",
  // and the corresponding "*d.exe" debug builds all sit in the same folder.
  // The rule is simply: toggle "x64" <-> "x86" in our own filename.
  std::string ResolveSiblingCoverageExecutable() const
  {
    char selfPathBuffer[MAX_PATH] = { 0 };
    DWORD len = GetModuleFileNameA(NULL, selfPathBuffer, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
    {
      return std::string();
    }

    std::string selfPath(selfPathBuffer, len);

    // Find the last "x64" or "x86" substring within the filename portion
    // only. Searching inside the directory portion would misidentify the
    // sibling when the build directory itself happens to contain "x64".
    auto slash = selfPath.find_last_of("\\/");
    size_t fileStart = (slash == std::string::npos) ? 0 : slash + 1;

    std::string filename = selfPath.substr(fileStart);

    auto lastX64 = filename.rfind("x64");
    auto lastX86 = filename.rfind("x86");

    // Also accept uppercase X64/X86 in case someone renames the binaries.
    if (lastX64 == std::string::npos)
    {
      lastX64 = filename.rfind("X64");
    }
    if (lastX86 == std::string::npos)
    {
      lastX86 = filename.rfind("X86");
    }

    std::string siblingFilename = filename;
    if (lastX64 != std::string::npos &&
        (lastX86 == std::string::npos || lastX64 > lastX86))
    {
      siblingFilename.replace(lastX64, 3,
        (filename[lastX64] == 'X') ? "X86" : "x86");
    }
    else if (lastX86 != std::string::npos)
    {
      siblingFilename.replace(lastX86, 3,
        (filename[lastX86] == 'X') ? "X64" : "x64");
    }
    else
    {
      // Our own executable name doesn't contain x64/x86; we can't
      // automatically find the sibling.
      return std::string();
    }

    std::string siblingPath = selfPath.substr(0, fileStart) + siblingFilename;
    if (!std::filesystem::exists(siblingPath))
    {
      return std::string();
    }
    return siblingPath;
  }

  // Try to hand off the given child process (identified by PID) to our
  // sibling-bitness CPPCoverage so it can be instrumented. Returns true on
  // success. On success, the sibling process handle is stored in
  // siblingCoverageProcesses and its output file in auxiliaryCoverageOutputs.
  //
  // IMPORTANT: the caller is responsible for subsequently detaching from the
  // child via DebugActiveProcessStop. This method intentionally does not do
  // the detach itself: that must happen in the top-level debug-event loop at
  // a well-defined point (after ContinueDebugEvent).
  bool SpawnSiblingCoverageForChild(DWORD childPid, const std::string& childExePath)
  {
    std::string siblingExe = ResolveSiblingCoverageExecutable();
    if (siblingExe.empty())
    {
      if (options.isAtLeastLevel(VerboseLevel::Warning))
      {
        std::cout << "No sibling CPPCoverage.exe found next to this executable; "
          "cannot instrument cross-bitness child PID " << childPid << "." << std::endl;
      }
      return false;
    }

    // Compute a unique output file for the sibling. We don't want it to
    // overwrite our own .cov. Main.cpp will later merge it into the
    // MergedOutput if one was requested.
    std::string auxOutput;
    if (!options.OutputFile.empty())
    {
      auxOutput = options.OutputFile + ".child-" + std::to_string(childPid) + ".cov";
    }
    else if (!childExePath.empty())
    {
      auxOutput = childExePath + ".child-" + std::to_string(childPid) + ".cov";
    }
    else
    {
      auxOutput = options.Executable + ".child-" + std::to_string(childPid) + ".cov";
    }

    // Build the sibling command line. Note we deliberately do NOT pass
    // -m: merging into a single merged file from multiple concurrent
    // coverage processes is racy. Main.cpp runs the merge for us after
    // all helpers have finished.
    std::string cmd = QuoteArg(siblingExe);
    cmd += " -quiet";
    cmd += " -attach ";
    cmd += std::to_string(childPid);
    cmd += " -o ";
    cmd += QuoteArg(auxOutput);

    switch (options.ExportFormat)
    {
    case RuntimeOptions::ExportFormatType::Native:     cmd += " -format native"; break;
    case RuntimeOptions::ExportFormatType::NativeV2:   cmd += " -format nativeV2"; break;
    case RuntimeOptions::ExportFormatType::Cobertura:  cmd += " -format cobertura"; break;
    case RuntimeOptions::ExportFormatType::Clover:     cmd += " -format clover"; break;
    }

    if (options.UseStaticCodeAnalysis)
    {
      cmd += " -codeanalysis";
    }
    if (options.ConsolidateAuxiliary)
    {
      // If a sibling itself has to hand off another cross-bitness
      // grandchild, we want that chain to also collapse to one file.
      cmd += " -consolidate";
    }
    for (const auto& codePath : options.CodePaths)
    {
      cmd += " -p " + QuoteArg(codePath);
    }
    if (!options.SolutionPath.empty())
    {
      cmd += " -solution " + QuoteArg(options.SolutionPath);
    }
    if (!options.PackageName.empty() && options.PackageName != "Program.exe")
    {
      cmd += " -pkg " + QuoteArg(options.PackageName);
    }

    if (!childExePath.empty())
    {
      cmd += " -- ";
      cmd += QuoteArg(childExePath);
    }

    if (options.isAtLeastLevel(VerboseLevel::Info))
    {
      std::cout << "Launching sibling coverage for cross-bitness PID "
        << childPid << ": " << cmd << std::endl;
    }

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // Share stdout/stderr so sibling diagnostics reach the same terminal.
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput  = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);

    // CreateProcess wants a writable buffer for the command line.
    std::vector<char> cmdBuffer(cmd.begin(), cmd.end());
    cmdBuffer.push_back('\0');

    BOOL ok = CreateProcessA(
      NULL,
      cmdBuffer.data(),
      NULL, NULL,
      TRUE,   // inherit handles (for stdout/stderr sharing)
      0,
      NULL, NULL,
      &si, &pi);

    if (!ok)
    {
      if (options.isAtLeastLevel(VerboseLevel::Error))
      {
        std::cerr << "Failed to launch sibling coverage process: "
          << Util::GetLastErrorAsString() << std::endl;
      }
      return false;
    }

    CloseHandle(pi.hThread);
    siblingCoverageProcesses.push_back(pi.hProcess);
    auxiliaryCoverageOutputs.push_back(auxOutput);
    return true;
  }

  std::string GetFileNameFromHandle(HANDLE hFile)
  {
    BOOL bSuccess = FALSE;
    TCHAR pszFilename[MAX_PATH + 1];
    HANDLE hFileMap;

    std::string strFilename;

    // Get the file size.
    DWORD dwFileSizeHi = 0;
    DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi);

    if (dwFileSizeLo == 0 && dwFileSizeHi == 0)
    {
      return std::string();
    }

    // Create a file mapping object.
    hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 1, NULL);

    if (hFileMap)
    {
      // Create a file mapping to get the file name.
      void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);

      if (pMem)
      {
        if (GetMappedFileName(GetCurrentProcess(), pMem, pszFilename, MAX_PATH))
        {
          // Translate path with device name to drive letters.
          TCHAR szTemp[512];
          szTemp[0] = '\0';

          if (GetLogicalDriveStrings(512 - 1, szTemp))
          {
            TCHAR szName[MAX_PATH];
            TCHAR szDrive[3] = TEXT(" :");
            BOOL bFound = FALSE;
            TCHAR* p = szTemp;

            do
            {
              // Copy the drive letter to the template string
              *szDrive = *p;

              // Look up each device name
              if (QueryDosDevice(szDrive, szName, MAX_PATH))
              {
                size_t uNameLen = strlen(szName);

                if (uNameLen < MAX_PATH)
                {
                  bFound = strncmp(pszFilename, szName, uNameLen) == 0;

                  if (bFound)
                  {
                    strFilename = szDrive;
                    strFilename += (pszFilename + uNameLen);
                  }
                }
              }

              // Go to the next NULL character.
              while (*p++)
                ;
            } while (!bFound && *p); // end of string
          }
        }
        bSuccess = TRUE;
        UnmapViewOfFile(pMem);
      }

      CloseHandle(hFileMap);
    }

    return strFilename;
  }

  void InitializeDebugInfo(HANDLE proc)
  {
    BOOL initSuccess = SymInitialize(proc, NULL, FALSE);
    debugInfoAvailable = (initSuccess == TRUE);
    if (!debugInfoAvailable)
    {
      if (options.isAtLeastLevel(VerboseLevel::Error))
      {
        std::cout << "Cannot calculate code coverage: debug info is not available." << std::endl;
      }
    }
  }

  void TryPatchDebuggerPresent(HANDLE proc, LPHANDLE fileHandle, PVOID basePtr, std::string filename)
  {
    auto idx = filename.find_last_of('\\');
    if (idx != std::string::npos)
    {
      filename = filename.substr(idx + 1);
    }

    if (Util::InvariantEquals(filename, "KernelBase.dll"))
    {
      if (options.isAtLeastLevel(VerboseLevel::Trace))
      {
        std::cout << "Patching IsDebuggerPresent() in KernelBase.dll" << std::endl;
      }

      // Find relative address of IsDebuggerPresent using our own version. We're sure it's the same, so why not:
      auto module = GetModuleHandle("KernelBase.dll");
      auto modAddr = reinterpret_cast<BYTE*>(module);
      auto fcnAddr = GetProcAddress(module, "IsDebuggerPresent");

      PVOID processAddress =
        reinterpret_cast<PVOID>(
          reinterpret_cast<BYTE*>(basePtr) +
          (reinterpret_cast<DWORD64>(fcnAddr) - reinterpret_cast<DWORD64>(modAddr))); // Yuck!

      // Patch the data:
      //
      // Signature of method:
      // BOOL __stdcall IsDebuggerPresent() { return false; }
      BYTE bytes[3] = // return false;
      {
        0x33, 0xC0, // xor eax, eax
        0xC3		// ret
      };

      // Write back the original data:
      SIZE_T written;
      WriteProcessMemory(proc, processAddress, bytes, 3, &written);
      FlushInstructionCache(proc, processAddress, 3);

      debuggerPresentPatched = true;
    }
  }

  void ProcessDebugInfo(ProcessInfo* proc, LPHANDLE fileHandle, PVOID basePtr, const std::string& filename)
  {
    if (debugInfoAvailable)
    {
      bool firstTimeLoad = false;
      if (loadedFiles.find(filename) == loadedFiles.end())
      {
        loadedFiles.insert(filename);
        firstTimeLoad = true;
      }

      DWORD64 dllBase;
      auto idx = proc->LoadedModules.find(basePtr);
      if (idx != proc->LoadedModules.end())
      {
        dllBase = idx->second;
      }
      else
      {
        dllBase = SymLoadModuleEx(proc->Handle, fileHandle, filename.c_str(), NULL, reinterpret_cast<DWORD64>(basePtr), 0, NULL, 0);
        proc->LoadedModules[basePtr] = dllBase;
      }

      if (dllBase)
      {
        IMAGEHLP_MODULE64 ModuleInfo;
        memset(&ModuleInfo, 0, sizeof(ModuleInfo));
        ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);

        BOOL info = SymGetModuleInfo64(proc->Handle, dllBase, &ModuleInfo);

        // Only register line numbers the first time. On a second load of the same DLL, we only want to set the breakpoints.
        CallbackInfo ci(&coverageContext, proc, firstTimeLoad);

        if (info)
        {
#ifdef _WIN64
          IMAGEHLP_SYMBOL64 img;
#else
          IMAGEHLP_SYMBOL img;
#endif

          if (SymGetSymFromName(proc->Handle, "PassToCPPCoverage", &img) && (coverageContext.GetFilename() == filename))
          {
            auto size = ReachabilityAnalysis::FirstInstructionSize(proc->Handle, img.Address);

            if (options.isAtLeastLevel(VerboseLevel::Trace))
            {
              std::cout << "Found pass method at 0x" << std::hex << img.Address << std::dec << " with next breakpoint at +" << size << std::endl;
            }

            BYTE buffer[2];
            SIZE_T numberRead;
            if (ReadProcessMemory(proc->Handle, reinterpret_cast<PVOID>(img.Address), buffer, 1, &numberRead) &&
                ReadProcessMemory(proc->Handle, reinterpret_cast<PVOID>(img.Address), buffer + 1, 1, &numberRead))
            {
              passToCoverageMethods.push_back(
                std::make_tuple(reinterpret_cast<PVOID>(img.Address), buffer[0],
                                reinterpret_cast<PVOID>(img.Address + size), buffer[1]));
            }
          }

          if (SymEnumLines(proc->Handle, dllBase, NULL, NULL, SymEnumLinesCallback, &ci))
          {
            if (!options.UseStaticCodeAnalysis ||
                !SymEnumSymbols(proc->Handle, dllBase, NULL, SymEnumSymbolsCallback, &ci) || ci.reachableCode.empty())
            {
              auto err = Util::GetLastErrorAsString();
              if (options.isAtLeastLevel(VerboseLevel::Info))
              {
                if (!options.UseStaticCodeAnalysis)
                {
                  std::cout << "[Symbols loaded]" << std::endl;
                }
                else
                {
                  std::cout << "[Symbols loaded, but static code analysis failed: " << err << "]" << std::endl;
                }
              }

              ci.SetBreakpoints(basePtr, proc->Handle);
            }
            else
            {
              if (options.isAtLeastLevel(VerboseLevel::Info))
              {
                std::cout << "[Symbols loaded]" << std::endl;
              }

              std::sort(ci.reachableCode.begin(), ci.reachableCode.end());

              std::set<PVOID> breakpointsToSet;
              size_t index = 0;
              for (auto it : ci.breakpointsToSet)
              {
                auto ptr = reinterpret_cast<size_t>(it);
                while (index < ci.reachableCode.size() &&
                       ptr > size_t(ci.reachableCode[index].methodStart + ci.reachableCode[index].numberBytes))
                {
                  ++index;
                }

                if (index < ci.reachableCode.size())
                {
                  auto& item = ci.reachableCode[index];
                  if (ptr >= item.methodStart && ptr < item.methodStart + item.numberBytes)
                  {
                    if (item.state[ptr - item.methodStart] & 0x10)
                    {
                      breakpointsToSet.insert(it);
                    }
                  }
                }
                else
                {
                  break;
                }
              }

              if (options.isAtLeastLevel(VerboseLevel::Trace))
              {
                std::cout << "[" << ci.breakpointsToSet.size() << " breakpoints total, " << breakpointsToSet.size() << " are reachable]" << std::endl;
              }
              swap(ci.breakpointsToSet, breakpointsToSet);

              ci.SetBreakpoints(basePtr, proc->Handle);
            }
          }
          else
          {
            if (options.isAtLeastLevel(VerboseLevel::Trace))
            {
              std::cout << "[No symbols available: " << Util::GetLastErrorAsString() << "]" << std::endl;
            }
          }
        }
        else
        {
          if (options.isAtLeastLevel(VerboseLevel::Trace))
          {
            std::cout << "[No symbol info found: " << Util::GetLastErrorAsString() << "]" << std::endl;
          }
        }
      }
      else
      {
        if (options.isAtLeastLevel(VerboseLevel::Trace))
        {
          std::cout << "[PDB not loaded: " << Util::GetLastErrorAsString() << "]" << std::endl;
        }
      }
    }
    else
    {
      if (options.isAtLeastLevel(VerboseLevel::Trace))
      {
        std::cout << std::endl;
      }
    }
  }

  static BOOL __stdcall ReadProcessMemoryInt(HANDLE process, DWORD64 baseAddr, PVOID buffer, DWORD size, LPDWORD numberBytesRead)
  {
    SIZE_T count;
    BOOL result = ReadProcessMemory(process, (PVOID) baseAddr, buffer, size, &count);
    *numberBytesRead = DWORD(count);
    return result;
  }

  bool Start()
  {
    SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_LOAD_ANYTHING);

    if (options.Attach)
    {
      // Attach mode: we're taking over an already-running process
      // (typically handed off from a sibling-bitness CPPCoverage).
      //
      // There is an inherent race when we're spawned as a handoff from
      // the sibling CPPCoverage: the sibling launches us before it
      // finishes detaching via DebugActiveProcessStop, so our first
      // DebugActiveProcess call may race against the sibling still
      // owning the debug port. We therefore retry briefly before
      // giving up. Under normal single-process usage, the first call
      // succeeds immediately.
      const int maxAttempts = 50;  // ~5 seconds total
      BOOL attached = FALSE;
      DWORD lastError = 0;
      for (int attempt = 0; attempt < maxAttempts && !attached; ++attempt)
      {
        attached = DebugActiveProcess(options.AttachPid);
        if (!attached)
        {
          lastError = GetLastError();
          // Abort immediately if the target doesn't exist anymore;
          // retrying won't help.
          if (lastError == ERROR_INVALID_PARAMETER)
          {
            break;
          }
          Sleep(100);
        }
      }

      if (!attached)
      {
        SetLastError(lastError);
        const std::string msg = "Error attaching to process " +
          std::to_string(options.AttachPid) + ": " +
          Util::GetLastErrorAsString();
        throw std::exception(msg.c_str());
      }

      // Make sure the target isn't killed if *we* crash. The sibling
      // that spawned us has its own expectations about the child
      // surviving long enough to finish the test run.
      DebugSetProcessKillOnExit(FALSE);

      if (options.isAtLeastLevel(VerboseLevel::Info))
      {
        std::cout << "Attached to PID " << options.AttachPid << std::endl;
      }
    }
    else
    {
      STARTUPINFO si;
      PROCESS_INFORMATION pi;
      ZeroMemory(&si, sizeof(si));
      si.cb = sizeof(si);
      ZeroMemory(&pi, sizeof(pi));

      std::string arguments;
      if (!options.ExecutableArguments.empty())
      {
        arguments = options.ExecutableArguments;
      }

      // Read working directory (if empty need to set NULL)
      const char* workingDirectory = NULL;
      if (!options.WorkingDirectory.empty())
      {
        workingDirectory = options.WorkingDirectory.c_str();
      }

      auto result = CreateProcess(NULL, arguments.data(), NULL, NULL, FALSE, DEBUG_PROCESS, NULL, workingDirectory, &si, &pi);
      if (result == 0)
      {
        if (pi.dwProcessId == 0)
        {
          if (options.isAtLeastLevel(VerboseLevel::Error))
          {
            const std::string msg = "Error running process; the most likely cause of this is a x86/x64 mix-up. Message " + Util::GetLastErrorAsString();
            throw std::exception(msg.c_str());
          }
        }
        else
        {
          if (options.isAtLeastLevel(VerboseLevel::Error))
          {
            const std::string msg = "Error running process: " + Util::GetLastErrorAsString();
            throw std::exception(msg.c_str());
          }
        }
      }
    }

    /*
    TODO FIXME:
    -----------

    Something fishy is going on here: apparently debuggers only like x86/x86 and x64/x64 combinations, probably due to
    context switches and so on. Fine as that is, you would expect IsWow64Process to give us this information... well,
    guess what: it doesn't. Currently, isX64 is always 0, even if we're running an x64 process in an x64 coverage instance.

    Code:
    -----

    BOOL isX64;
    if (!IsWow64Process(pi.hProcess, &isX64))
    {
      std::cout << "Cannot determine if process is x64 or x86. Aborting." << std::endl;
      return;
    }

    if (isX64 == FALSE)
    {
#ifdef _WIN64
      std::cout << "You cannot run a x86 child process in a x64 coverage process." << std::endl;
      return;
#endif
    }
    else
    {
#ifdef _WIN32
      std::cout << "You cannot run a x64 child process in a x86 coverage process." << std::endl;
      return;
#endif
    }
    */

    std::unordered_map<LPVOID, std::string> dllNameMap;

    bool continueDebugging = true;
    DWORD continueStatus = DBG_CONTINUE;
    DEBUG_EVENT debugEvent = { 0 };

    bool entryBreakpoint = true;
    bool initializedDbgInfo = false;

    // Check if all process works
    bool executionSuccess = true;

    std::unordered_map<DWORD, std::unique_ptr<ProcessInfo>> processMap;

    while (continueDebugging)
    {
      if (!WaitForDebugEvent(&debugEvent, 500))
      {
        // Collect sample:
        for (auto& proc : processMap)
        {
          DebugBreakProcess(proc.second->Handle);
        }
      }
      else
      {
        switch (debugEvent.dwDebugEventCode)
        {
          case CREATE_PROCESS_DEBUG_EVENT:
          {
            auto process = debugEvent.u.CreateProcessInfo.hProcess;
            auto thread = debugEvent.u.CreateProcessInfo.hThread;

            // Cross-bitness children (e.g. a 64-bit vstest.console.exe
            // spawning a 32-bit testhost.x86.exe) cannot be safely
            // instrumented by this debugger: loading symbols and
            // writing 0xCC breakpoints across the bitness boundary
            // corrupts them. Hand them off to the sibling-bitness
            // CPPCoverage.exe (which ships alongside this binary) via
            // process attach, then detach ourselves so the sibling can
            // take over as the sole debugger.
            if (!IsProcessCompatibleBitness(process))
            {
              auto filename = GetFileNameFromHandle(debugEvent.u.CreateProcessInfo.hFile);
              if (options.isAtLeastLevel(VerboseLevel::Info))
              {
                std::cout << "Cross-bitness child detected: "
                  << filename << " (PID " << debugEvent.dwProcessId
                  << "). Handing off to sibling coverage tool." << std::endl;
              }

              // The debugger owns hFile on CREATE_PROCESS_DEBUG_EVENT
              // and must close it. Since we won't be loading symbols
              // from it, release it now to avoid holding a file lock.
              if (debugEvent.u.CreateProcessInfo.hFile != NULL)
              {
                CloseHandle(debugEvent.u.CreateProcessInfo.hFile);
              }

              // Try to launch the sibling CPPCoverage before we
              // detach. Even though the child is still suspended at
              // this point (we haven't ContinueDebugEvent'd yet),
              // the sibling can't attach while we're still the
              // debugger, so the actual DebugActiveProcess call in
              // the sibling will happen after our detach. There is
              // a brief window during which the child runs
              // unmonitored; Windows mitigates this via synthetic
              // LOAD_DLL / CREATE_PROCESS events when the sibling
              // attaches, so no already-loaded module is missed.
              SpawnSiblingCoverageForChild(debugEvent.dwProcessId, filename);

              // Defer the actual detach until after ContinueDebugEvent
              // for this event has been issued.
              pendingDetach.push_back(debugEvent.dwProcessId);
              break;
            }

            auto pinfo = new ProcessInfo(debugEvent.dwProcessId, process);
            pinfo->Threads[debugEvent.dwThreadId] = thread;
            processMap[debugEvent.dwProcessId] = std::unique_ptr<ProcessInfo>(pinfo);

            auto filename = GetFileNameFromHandle(debugEvent.u.CreateProcessInfo.hFile);
            if (options.isAtLeastLevel(VerboseLevel::Info))
            {
              std::cout << "Loading process: " << filename << "... ";
            }

            InitializeDebugInfo(process);
            initializedDbgInfo = true;

            ProcessDebugInfo(pinfo, &(debugEvent.u.CreateProcessInfo.hFile), debugEvent.u.CreateProcessInfo.lpBaseOfImage, filename);
          }
          break;

          case CREATE_THREAD_DEBUG_EVENT:
          {
            auto thread = debugEvent.u.CreateThread.hThread;
            auto proc = processMap[debugEvent.dwProcessId].get();
            proc->Threads[debugEvent.dwThreadId] = thread;
          }
          break;

          case EXIT_THREAD_DEBUG_EVENT:
          {
            auto proc = processMap[debugEvent.dwProcessId].get();
            proc->Threads.erase(proc->Threads.find(debugEvent.dwThreadId));
          }
          break;

          case EXIT_PROCESS_DEBUG_EVENT:
          {
            if (options.isAtLeastLevel(VerboseLevel::Info))
            {
              std::cout << "Process exited with code: " << debugEvent.u.ExitProcess.dwExitCode << "." << std::endl;
            }

            // Success application must return 0 --> Commented out per PR #97.
            // executionSuccess &= (debugEvent.u.ExitProcess.dwExitCode == 0);

            processMap.erase(debugEvent.dwProcessId);

            // Get only the latest RC code of latest process.
            // Here we consider all process depends of master process which must be released at the end.
            if (processMap.empty())
            {
              executionSuccess = (debugEvent.u.ExitProcess.dwExitCode == 0);
            }
            continueDebugging = processMap.empty() ? false : true;
          }
          break;

          case LOAD_DLL_DEBUG_EVENT:
          {
            auto name = GetFileNameFromHandle(debugEvent.u.LoadDll.hFile);
            if (options.isAtLeastLevel(VerboseLevel::Info))
            {
              std::cout << "Loading: " << name << "... " << std::endl;
            }

            dllNameMap[debugEvent.u.LoadDll.lpBaseOfDll] = name;

            auto process = processMap[debugEvent.dwProcessId].get();

            ProcessDebugInfo(process, &(debugEvent.u.LoadDll.hFile), debugEvent.u.LoadDll.lpBaseOfDll, name);
            TryPatchDebuggerPresent(process->Handle, &(debugEvent.u.LoadDll.hFile), debugEvent.u.LoadDll.lpBaseOfDll, name);
          }
          break;

          case UNLOAD_DLL_DEBUG_EVENT:
          {
            auto basePtr = debugEvent.u.UnloadDll.lpBaseOfDll;
            auto idx = dllNameMap.find(basePtr);
            if (idx != dllNameMap.end())
            {
              if (options.isAtLeastLevel(VerboseLevel::Info))
              {
                std::cout << "Unloading: " << idx->second << std::endl;
              }

              // Unload symbols module:
              auto process = processMap[debugEvent.dwProcessId].get();
              auto mod = process->LoadedModules.find(basePtr);
              if (mod != process->LoadedModules.end())
              {
                BOOL result = SymUnloadModule64(process->Handle, mod->second);
                if (!result)
                {
                  if (options.isAtLeastLevel(VerboseLevel::Info))
                  {
                    std::cout << "Unloading module failed: " << Util::GetLastErrorAsString() << std::endl;
                  }
                }

                process->LoadedModules.erase(mod);
              }

              // Remove from DLL map.
              dllNameMap.erase(idx);
            }
            else
            {
              if (options.isAtLeastLevel(VerboseLevel::Trace))
              {
                std::cout << "Unloading: ???." << std::endl;
              }
            }
          }
          break;

          case OUTPUT_DEBUG_STRING_EVENT:
          {
            // ignore
          }
          break;

          case EXCEPTION_DEBUG_EVENT:
          {
            EXCEPTION_DEBUG_INFO& exception = debugEvent.u.Exception;
            switch (exception.ExceptionRecord.ExceptionCode)
            {
              case STATUS_BREAKPOINT:

                if (entryBreakpoint)
                {
                  // std::cout << "Entry breakpoint; ignoring." << std::endl;
                  entryBreakpoint = false;
                }
                else
                {
                  auto process = processMap[debugEvent.dwProcessId].get();
                  auto thread = process->Threads[debugEvent.dwThreadId];

                  // std::cout << "Breakpoint hit!" << std::endl;

                  // Undo our breakpoint:
                  CONTEXT threadContextInfo;
                  threadContextInfo.ContextFlags = CONTEXT_ALL;
                  GetThreadContext(thread, &threadContextInfo);

#if _WIN64
                  threadContextInfo.Rip--;
#else
                  threadContextInfo.Eip--;
#endif
                  auto addr = exception.ExceptionRecord.ExceptionAddress;

                  bool found = false;

                  // Is this a breakpoint in one of the 'pass' functions?
                  for (auto& it : passToCoverageMethods)
                  {
                    if (addr == std::get<0>(it))
                    {
#if _WIN64
                      auto numberBytes = threadContextInfo.Rcx;
                      auto pointer = threadContextInfo.Rdx;
#else
                      auto numberBytes = threadContextInfo.Ecx;
                      auto pointer = threadContextInfo.Eax;
#endif
                      {
                        auto data = std::make_unique<char[]>(numberBytes + 1);
                        SIZE_T numberBytesRead;
                        ReadProcessMemory(process->Handle, reinterpret_cast<LPVOID>(pointer), data.get(), numberBytes, &numberBytesRead);
                        data[numberBytesRead] = 0;

                        if (options.isAtLeastLevel(VerboseLevel::Trace))
                        {
                          std::cout << "Child process notification: " << data << std::endl;

                          notifications.Handle(data.get(), numberBytesRead);
                        }
                      }

                      // Reset the first breakpoint, set the second breakpoint
                      SIZE_T written;
                      BYTE orig = std::get<1>(it);
                      BYTE buffer = 0xCC;

                      WriteProcessMemory(process->Handle, std::get<0>(it), &orig, 1, &written);
                      FlushInstructionCache(process->Handle, std::get<0>(it), 1);

                      WriteProcessMemory(process->Handle, std::get<2>(it), &buffer, 1, &written);
                      FlushInstructionCache(process->Handle, std::get<2>(it), 1);

                      found = true;

                      // Make sure to 'hit' this breakpoint if necessary:
                      auto bp = process->breakPoints.find(std::get<0>(it));
                      if (bp != process->breakPoints.end())
                      {
                        // Set the fact that it's a hit:
                        bp->second.lineInfo->HitCount++;
                      }
                    }
                    else if (addr == std::get<2>(it))
                    {
                      // Reset the second breakpoint, set the firstbreakpoint
                      SIZE_T written;
                      BYTE buffer = 0xCC;
                      BYTE orig = std::get<3>(it);

                      WriteProcessMemory(process->Handle, std::get<0>(it), &buffer, 1, &written);
                      FlushInstructionCache(process->Handle, std::get<0>(it), 1);

                      WriteProcessMemory(process->Handle, std::get<2>(it), &orig, 1, &written);
                      FlushInstructionCache(process->Handle, std::get<2>(it), 1);

                      found = true;

                      // Make sure to 'hit' this breakpoint if necessary:
                      auto bp = process->breakPoints.find(std::get<2>(it));
                      if (bp != process->breakPoints.end())
                      {
                        // Set the fact that it's a hit:
                        bp->second.lineInfo->HitCount++;
                      }
                    }
                  }

                  if (found)
                  {
                    // Restore context
                    SetThreadContext(thread, &threadContextInfo);
                  }
                  else
                  {
                    auto bp = process->breakPoints.find(addr);
                    if (bp != process->breakPoints.end())
                    {
                      // Write back the original data:
                      SIZE_T written;
                      WriteProcessMemory(process->Handle, addr, &bp->second.originalData, 1, &written);
                      FlushInstructionCache(process->Handle, addr, 1);

                      // Set the fact that it's a hit:
                      bp->second.lineInfo->HitCount++;

                      // Restore context
                      SetThreadContext(thread, &threadContextInfo);
                    }
                    else
                    {
                      // We don't need to restore the 0xCC; best to fix the RIP/EIP.
#if _WIN64
                      threadContextInfo.Rip++;
#else
                      threadContextInfo.Eip++;
#endif
                      // Usually, a breakpoint is *not* a DebugBreak but rather one of our suspend calls.
                      // Iterate all threads, get stack traces:
                      for (auto& threadPair : process->Threads)
                      {
                        // If the thread is the breaking thread - then we're not interested.
                        if (threadPair.first == debugEvent.dwThreadId)
                        {
                          continue;
                        }

                        CONTEXT threadContextInfo;
                        threadContextInfo.ContextFlags = CONTEXT_ALL;
                        GetThreadContext(threadPair.second, &threadContextInfo);

#if _WIN64
                        STACKFRAME64 stack = { 0 };
                        stack.AddrPC.Offset = threadContextInfo.Rip; // EIP - Instruction Pointer
                        stack.AddrPC.Mode = AddrModeFlat;
                        stack.AddrFrame.Offset = threadContextInfo.Rsp; // ESP - Stack Pointer
                        stack.AddrFrame.Mode = AddrModeFlat;
                        stack.AddrStack.Offset = threadContextInfo.Rsp; // ESP - Stack Pointer (again!)
                        stack.AddrStack.Mode = AddrModeFlat;
#else
                        STACKFRAME64 stack = { 0 };
                        stack.AddrPC.Offset = threadContextInfo.Eip; // EIP - Instruction Pointer
                        stack.AddrPC.Mode = AddrModeFlat;
                        stack.AddrFrame.Offset = threadContextInfo.Ebp; // EBP
                        stack.AddrFrame.Mode = AddrModeFlat;
                        stack.AddrStack.Offset = threadContextInfo.Esp; // ESP - Stack Pointer
                        stack.AddrStack.Mode = AddrModeFlat;
#endif

                        // Let's initialize this once for our process.
                        static SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO*>(calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1));
                        symbol->MaxNameLen = 255;
                        symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

                        BOOL status = TRUE;

                        std::vector<std::tuple<std::string, DWORD64, std::string>> callStack;

                        do
                        {
                          // Process stack item:
                          std::ostringstream oss;

                          if (!SymFromAddr(process->Handle, stack.AddrPC.Offset, 0, symbol))
                          {
                            // Ignore; no source
                          }
                          else
                          {
                            DWORD dwDisplacement;
                            IMAGEHLP_LINE64 line;

                            line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

                            // Get information from PC
                            if (SymGetLineFromAddr64(process->Handle, stack.AddrPC.Offset, &dwDisplacement, &line))
                            {
                              if (coverageContext.PathMatches(line.FileName))
                              {
                                std::string filename(line.FileName);

                                callStack.push_back(std::make_tuple(filename, line.LineNumber, symbol->Name));
                              }
                            }
                          }

                          // Get next item from stack trace:
                          status = StackWalk64(IMAGE_FILE_MACHINE_AMD64, process->Handle, thread, &stack,
                                               &threadContextInfo, ReadProcessMemoryInt, SymFunctionTableAccess64,
                                               SymGetModuleBase64, 0);
                        } while (status);

                        // Update the profile graph:
                        for (size_t i = callStack.size(); i > 0; --i)
                        {
                          auto& item = callStack[i - 1];
                          auto line = std::get<1>(item);
                          auto frame = std::get<2>(item);

                          auto it = profileInfo.find(frame);
                          if (it == profileInfo.end())
                          {
                            ProfileFrame* frameInfo = new ProfileFrame(std::get<0>(item), line, i == 1);
                            profileInfo[frame] = std::unique_ptr<ProfileFrame>(frameInfo);
                          }
                          else
                          {
                            it->second->Update(line, i == 1);
                          }
                        }
                      }
                    }
                  }
                }

                break;

              default:
                //if (exception.dwFirstChance == 1)
                //{
                //   // ignore first chance (SEH) exception.
                //}
                // ...otherwise we *definitely* want to let the OS to handle it.

                continueStatus = DBG_EXCEPTION_NOT_HANDLED;
            }

            break;
          }
        }

        ContinueDebugEvent(debugEvent.dwProcessId, debugEvent.dwThreadId, continueStatus);

        continueStatus = DBG_CONTINUE;

        // Detach any child processes that were flagged during this
        // event cycle (e.g. cross-bitness children). This is done
        // after ContinueDebugEvent so the kernel has fully processed
        // the CREATE_PROCESS event for the child before we tell it we
        // no longer want to debug it.
        if (!pendingDetach.empty())
        {
          for (DWORD detachPid : pendingDetach)
          {
            if (!DebugActiveProcessStop(detachPid))
            {
              if (options.isAtLeastLevel(VerboseLevel::Warning))
              {
                std::cerr << "Warning: failed to detach from PID "
                  << detachPid << ": "
                  << Util::GetLastErrorAsString() << std::endl;
              }
            }
          }
          pendingDetach.clear();
        }
      }
    }

    if (initializedDbgInfo)
    {
      for (auto& it : processMap)
      {
        SymCleanup(it.second->Handle);
      }
    }

    // Wait for sibling CPPCoverage helper processes (spawned to handle
    // cross-bitness children) to finish. They each write their own .cov
    // file which Main.cpp will fold into the merged coverage output.
    // By the time we get here our primary target has already exited, so
    // its cross-bitness workers should be exiting or already gone.
    if (!siblingCoverageProcesses.empty())
    {
      if (options.isAtLeastLevel(VerboseLevel::Info))
      {
        std::cout << "Waiting for " << siblingCoverageProcesses.size()
          << " sibling coverage helper process(es)..." << std::endl;
      }

      for (HANDLE h : siblingCoverageProcesses)
      {
        WaitForSingleObject(h, INFINITE);

        DWORD exitCode = 0;
        if (GetExitCodeProcess(h, &exitCode) && exitCode != 0)
        {
          if (options.isAtLeastLevel(VerboseLevel::Warning))
          {
            std::cerr << "Sibling coverage helper exited with code "
              << exitCode << std::endl;
          }
          // Don't fail the whole run on a sibling failure: the user
          // still gets our own coverage, and the sibling issue is
          // surfaced via its non-zero exit code in the log.
        }
        CloseHandle(h);
      }
      siblingCoverageProcesses.clear();
    }

    // Group profile data together:
    if (options.isAtLeastLevel(VerboseLevel::Trace))
    {
      std::cout << "Gathering profile data of " << profileInfo.size() << " sources..." << std::endl;
    }
    std::unordered_map<std::string, std::unique_ptr<std::vector<ProfileInfo>>> mergedInfo;

    float totalDeep = 0;
    for (auto& it : profileInfo)
    {
      for (auto& jt : it.second->lineHitCount)
      {
        totalDeep += jt.second.Deep;
      }
    }

    if (totalDeep == 0)
    {
      totalDeep = 1;
    }

    for (auto& it : profileInfo)
    {
      DWORD64 max = 0;
      float totalShallow = 0;
      for (auto& jt : it.second->lineHitCount)
      {
        if (jt.first > max)
        {
          max = jt.first;
        }

        totalShallow += jt.second.Shallow;
      }

      if (totalShallow == 0)
      {
        totalShallow = 1;
      }

      auto m = mergedInfo.find(it.second->filename);
      std::vector<ProfileInfo>* merged;
      if (m == mergedInfo.end())
      {
        merged = new std::vector<ProfileInfo>();
        mergedInfo[it.second->filename] = std::unique_ptr<std::vector<ProfileInfo>>(merged);
      }
      else
      {
        merged = m->second.get();
      }

      if (merged->size() <= max)
      {
        merged->resize(size_t(max) + 1);
      }

      for (auto& jt : it.second->lineHitCount)
      {
        float deep = (jt.second.Deep / totalDeep) * 100.0f;
        float shallow = (jt.second.Shallow / totalShallow) * 100.0f;

        deep = (deep < 0) ? 0 : deep;
        shallow = (shallow < 0) ? 0 : shallow;

        (*merged)[size_t(jt.first)].Deep += deep;
        (*merged)[size_t(jt.first)].Shallow += shallow;
      }
    }

    if (options.isAtLeastLevel(VerboseLevel::Trace))
    {
      std::cout << "Filtering post-process notifications..." << std::endl;
    }

    coverageContext.Filter(notifications);

    if (options.isAtLeastLevel(VerboseLevel::Trace))
    {
      std::cout << "Writing coverage report..." << std::flush;
    }

    auto outputFile = options.OutputFile;
    if (outputFile.empty())
    {
      outputFile = options.Executable + ".cov";
    }

    // Write report of current execution
    {
      std::ofstream ofs(outputFile);
      coverageContext.WriteReport(options.ExportFormat, mergedInfo, ofs);
    }

    if (options.isAtLeastLevel(VerboseLevel::Info))
    {
      std::cout << "done." << std::endl;
    }

    return executionSuccess;
  }
};
