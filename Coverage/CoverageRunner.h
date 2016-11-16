#pragma once

#include "FileCallbackInfo.h"
#include "RuntimeOptions.h"
#include "Util.h"

#include <string>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

#include <Windows.h>
#include <psapi.h>
#include <DbgHelp.h>

struct CoverageRunner
{
	CoverageRunner(RuntimeOptions& opts) :
		options(opts),
		debugInfoAvailable(false),
		debuggerPresentPatched(false),
		coverageContext(opts.Executable, 0)
	{
		quiet = opts.Quiet;
	}

	static BOOL CALLBACK SymEnumLinesCallback(PSRCCODEINFO lineInfo, PVOID userContext)
	{
		FileCallbackInfo* info = reinterpret_cast<FileCallbackInfo*>(userContext);

		if (info->PathMatches(lineInfo->FileName))
		{
			auto file = lineInfo->FileName;

			PVOID addr = reinterpret_cast<PVOID>(lineInfo->Address);
			auto it = info->breakPoints.find(addr);
			if (it == info->breakPoints.end())
			{
				// Find line info
				auto fileLineInfo = info->LineInfo(file, lineInfo->LineNumber);
				if (fileLineInfo)
				{
					// Only create breakpoint if we haven't already.
					if (info->breakpointsToSet.insert(addr).second)
					{
						if (info->registerLines)
						{
							fileLineInfo->DebugCount++;
						}

						info->breakPoints.insert(std::make_pair(addr, BreakpointData('\0', fileLineInfo)));
					}
				}
			}
		}

		return TRUE;
	}

	RuntimeOptions& options;
	bool quiet;
	bool debugInfoAvailable;
	bool debuggerPresentPatched;
	std::unordered_set<std::string> loadedFiles;
	FileCallbackInfo coverageContext;

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
							while (*p++);
						}
						while (!bFound && *p); // end of string
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
		BOOL initSuccess = SymInitialize(proc, NULL, false);
		debugInfoAvailable = (initSuccess == TRUE);
		if (!debugInfoAvailable)
		{
			if (!quiet)
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
			if (!quiet)
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
				0xC3        // ret
			};

			// Write back the original data:
			SIZE_T written;
			WriteProcessMemory(proc, processAddress, bytes, 3, &written);
			FlushInstructionCache(proc, processAddress, 3);

			debuggerPresentPatched = true;
		}
	}

	void ProcessDebugInfo(HANDLE proc, LPHANDLE fileHandle, PVOID basePtr, const std::string& filename)
	{
		if (debugInfoAvailable)
		{
			bool firstTimeLoad = false;
			if (loadedFiles.find(filename) == loadedFiles.end())
			{
				loadedFiles.insert(filename);
				firstTimeLoad = true;
			}

			// Only register line numbers the first time. On a second load of the same DLL, we only want to set the breakpoints.
			coverageContext.registerLines = firstTimeLoad;

			auto dllBase = SymLoadModuleEx(proc, fileHandle, filename.c_str(), NULL, reinterpret_cast<DWORD64>(basePtr), 0, NULL, 0);

			if (dllBase)
			{
				IMAGEHLP_MODULE64 ModuleInfo;
				memset(&ModuleInfo, 0, sizeof(ModuleInfo));
				ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);

				BOOL info = SymGetModuleInfo64(proc, dllBase, &ModuleInfo);

				if (info)
				{
					if (SymEnumLines(proc, dllBase, NULL, NULL, SymEnumLinesCallback, &coverageContext))
					{
						if (!quiet)
						{
							std::cout << "[Symbols loaded]" << std::endl;
						}
						coverageContext.SetBreakpoints(basePtr, proc);
					}
					else
					{
						if (!quiet)
						{
							std::cout << "[No symbols available]" << std::endl;
						}
					}
				}
				else
				{
					if (!quiet)
					{
						std::cout << "[No symbol info found: " << Util::GetLastErrorAsString() << "]" << std::endl;
					}
				}
			}
			else
			{
				if (!quiet)
				{
					std::cout << "[PDB not loaded: " << Util::GetLastErrorAsString() << "]" << std::endl;
				}
			}

			BOOL result = SymUnloadModule64(proc, dllBase);
			if (!result)
			{
				if (!quiet)
				{
					std::cout << "Unloading module failed: " << Util::GetLastErrorAsString() << std::endl;
				}
			}
		}
		else
		{
			if (!quiet)
			{
				std::cout << std::endl;
			}
		}
	}

	void Start()
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		LPSTR args = NULL;
		if (options.ExecutableArguments.size() > 0)
		{
			args = &*options.ExecutableArguments.begin();
		}

		auto result = CreateProcess(options.Executable.c_str(), args, NULL, NULL, FALSE, DEBUG_ONLY_THIS_PROCESS, NULL, NULL, &si, &pi);
		if (result == 0)
		{
			if (pi.dwProcessId == 0)
			{
				if (!quiet)
				{
					std::cout << "Error running process; the most likely cause of this is a x86/x64 mix-up. Message: " << Util::GetLastErrorAsString() << std::endl;
				}
			}
			else
			{
				if (!quiet)
				{
					std::cout << "Error running process: " << Util::GetLastErrorAsString() << std::endl;
				}
			}
			return;
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

		while (continueDebugging)
		{
			if (!WaitForDebugEvent(&debugEvent, INFINITE))
			{
				return;
			}

			switch (debugEvent.dwDebugEventCode)
			{
				case CREATE_PROCESS_DEBUG_EVENT:
				{
					auto filename = GetFileNameFromHandle(debugEvent.u.CreateProcessInfo.hFile);
					if (!quiet)
					{
						std::cout << "Loading process: " << filename << "... ";
					}

					InitializeDebugInfo(pi.hProcess);
					initializedDbgInfo = true;
					coverageContext.processBasePtr = debugEvent.u.CreateProcessInfo.lpBaseOfImage;

					ProcessDebugInfo(pi.hProcess, &(debugEvent.u.CreateProcessInfo.hFile), debugEvent.u.CreateProcessInfo.lpBaseOfImage, filename);
				}
				break;

				case CREATE_THREAD_DEBUG_EVENT:
				{
					if (!quiet)
					{
						std::cout << "Thread 0x" << std::hex << debugEvent.u.CreateThread.hThread << std::dec << " (Id: " << debugEvent.dwThreadId << ") created." << std::endl;
					}
				}
				break;

				case EXIT_THREAD_DEBUG_EVENT:
				{
					if (!quiet)
					{
						std::cout << "Thread Id: " << debugEvent.dwThreadId << " exited with code: " << debugEvent.u.ExitThread.dwExitCode << "." << std::endl;
					}
				}
				break;

				case EXIT_PROCESS_DEBUG_EVENT:
				{
					if (!quiet)
					{
						std::cout << "Process exited with code: " << debugEvent.u.ExitProcess.dwExitCode << "." << std::endl;
					}
					continueDebugging = false;
				}
				break;

				case LOAD_DLL_DEBUG_EVENT:
				{
					auto name = GetFileNameFromHandle(debugEvent.u.LoadDll.hFile);
					if (!quiet)
					{
						std::cout << "Loading: " << name << "... ";
					}

					dllNameMap[debugEvent.u.LoadDll.lpBaseOfDll] = name;

					ProcessDebugInfo(pi.hProcess, &(debugEvent.u.LoadDll.hFile), debugEvent.u.LoadDll.lpBaseOfDll, name);

					TryPatchDebuggerPresent(pi.hProcess, &(debugEvent.u.LoadDll.hFile), debugEvent.u.LoadDll.lpBaseOfDll, name);
				}
				break;

				case UNLOAD_DLL_DEBUG_EVENT:
				{
					auto basePtr = debugEvent.u.UnloadDll.lpBaseOfDll;
					auto idx = dllNameMap.find(basePtr);
					if (idx != dllNameMap.end())
					{
						if (!quiet)
						{
							std::cout << "Unloading: " << idx->second << std::endl;
						}
						dllNameMap.erase(idx);
					}
					else
					{
						if (!quiet)
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
								// std::cout << "Breakpoint hit!" << std::endl;

								// Undo our breakpoint:
								CONTEXT threadContextInfo;
								threadContextInfo.ContextFlags = CONTEXT_ALL;
								GetThreadContext(pi.hThread, &threadContextInfo);

#if _WIN64
								threadContextInfo.Rip--;
#else
								threadContextInfo.Eip--;
#endif
								auto addr = exception.ExceptionRecord.ExceptionAddress;
								// std::cout << "Reset breakpoint at instruction " << std::hex << addr << std::dec << std::endl;

								auto bp = coverageContext.breakPoints.find(addr);
								if (bp != coverageContext.breakPoints.end())
								{
									// Write back the original data:
									SIZE_T written;
									WriteProcessMemory(pi.hProcess, addr, &bp->second.originalData, 1, &written);
									FlushInstructionCache(pi.hProcess, addr, 1);

									// Set the fact that it's a hit:
									bp->second.lineInfo->HitCount++;

									// Restore context
									SetThreadContext(pi.hThread, &threadContextInfo);
								}
								else
								{
									if (!quiet)
									{
										std::cout << "DebugBreak encountered in program; ignoring." << std::endl;
									}
#if _WIN64
									threadContextInfo.Rip++;
#else
									threadContextInfo.Eip++;
#endif
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
		}

		if (initializedDbgInfo)
		{
			SymCleanup(&(pi.hProcess));
		}

		if (!quiet)
		{
			std::cout << "Writing coverage report..." << std::flush;
		}

		auto outputFile = options.OutputFile;
		if (outputFile.size() == 0)
		{
			outputFile = options.Executable + ".cov";
		}

		coverageContext.WriteReport(options.ExportFormat, outputFile);

		if (!quiet)
		{
			std::cout << "done." << std::endl;
		}
	}
};
