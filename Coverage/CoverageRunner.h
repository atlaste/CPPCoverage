#pragma once

#include "FileCallbackInfo.h"
#include "RuntimeOptions.h"
#include "RuntimeNotifications.h"
#include "CallbackInfo.h"
#include "ProfileNode.h"
#include "Util.h"

#include "Disassembler/ReachabilityAnalysis.h"

#include <string>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

#include <Windows.h>
#include <psapi.h>

#pragma warning(disable: 4091)
#include <DbgHelp.h>
#pragma warning(default: 4091)

struct CoverageRunner
{
	CoverageRunner(RuntimeOptions& opts) :
		options(opts),
		debugInfoAvailable(false),
		debuggerPresentPatched(false),
		coverageContext(opts.Executable),
		profileInfo()
	{
		quiet = opts.Quiet;
	}

	static BOOL CALLBACK SymEnumLinesCallback(PSRCCODEINFO lineInfo, PVOID userContext)
	{
		CallbackInfo* info = reinterpret_cast<CallbackInfo*>(userContext);

		if (info->fileInfo->PathMatches(lineInfo->FileName))
		{
			auto file = lineInfo->FileName;

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

	RuntimeOptions& options;
	RuntimeNotifications notifications;
	bool quiet;
	bool debugInfoAvailable;
	bool debuggerPresentPatched;
	std::unordered_set<std::string> loadedFiles;
	FileCallbackInfo coverageContext;

	std::vector<std::tuple<PVOID, BYTE, PVOID, BYTE>> passToCoverageMethods;

	std::unordered_map<std::string, std::unique_ptr<ProfileFrame> > profileInfo;

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
		BOOL initSuccess = SymInitialize(proc, NULL, FALSE);
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

					if (SymGetSymFromName(proc->Handle, "PassToCPPCoverage", &img))
					{
						auto size = ReachabilityAnalysis::FirstInstructionSize(proc->Handle, img.Address);

						if (!quiet)
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
							!SymEnumSymbols(proc->Handle, dllBase, NULL, SymEnumSymbolsCallback, &ci) || ci.reachableCode.size() == 0)
						{
							auto err = Util::GetLastErrorAsString();
							if (!quiet)
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
							if (!quiet)
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

							std::cout << "[" << ci.breakpointsToSet.size() << " breakpoints total, " << breakpointsToSet.size() << " are reachable]" << std::endl;
							swap(ci.breakpointsToSet, breakpointsToSet);

							ci.SetBreakpoints(basePtr, proc->Handle);
						}
					}
					else
					{
						if (!quiet)
						{
							std::cout << "[No symbols available: " << Util::GetLastErrorAsString() << "]" << std::endl;
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
		}
		else
		{
			if (!quiet)
			{
				std::cout << std::endl;
			}
		}
	}

	static BOOL __stdcall ReadProcessMemoryInt(HANDLE process, DWORD64 baseAddr, PVOID buffer, DWORD size, LPDWORD numberBytesRead)
	{
		SIZE_T count;
		BOOL result = ReadProcessMemory(process, (PVOID)baseAddr, buffer, size, &count);
		*numberBytesRead = DWORD(count);
		return result;
	}

	void Start()
	{
		SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_LOAD_ANYTHING);

		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));

		LPSTR args = NULL;
		if( !options.ExecutableArguments.empty() )
		{
			args = &*options.ExecutableArguments.begin();
		}

        // Read working directory (if empty need to set NULL)
        const char* workingDirectory = NULL;
        if( !options.WorkingDirectory.empty() )
        {
            workingDirectory = options.WorkingDirectory.c_str();
        }

		auto result = CreateProcess(NULL, args, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, workingDirectory, &si, &pi);
		if (result == 0)
		{
			if (pi.dwProcessId == 0)
			{
				if (!quiet)
				{
                    const std::string msg = "Error running process; the most likely cause of this is a x86/x64 mix-up. Message " + Util::GetLastErrorAsString();
                    throw std::exception(msg.c_str());
				}
			}
			else
			{
				if (!quiet)
				{
                    const std::string msg = "Error running process: " + Util::GetLastErrorAsString();
					throw std::exception(msg.c_str());
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
						auto pinfo = new ProcessInfo(debugEvent.dwProcessId, process);
						pinfo->Threads[debugEvent.dwThreadId] = thread;
						processMap[debugEvent.dwProcessId] = std::unique_ptr<ProcessInfo>(pinfo);

						auto filename = GetFileNameFromHandle(debugEvent.u.CreateProcessInfo.hFile);
						if (!quiet)
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
						//if (!quiet)
						//{
						//	std::cout << "Thread 0x" << std::hex << debugEvent.u.CreateThread.hThread << std::dec << " (Id: " << debugEvent.dwThreadId << ") created." << std::endl;
						//}

						auto thread = debugEvent.u.CreateThread.hThread;
						auto proc = processMap[debugEvent.dwProcessId].get();
						proc->Threads[debugEvent.dwThreadId] = thread;
					}
					break;

					case EXIT_THREAD_DEBUG_EVENT:
					{
						//if (!quiet)
						//{
						//	std::cout << "Thread Id: " << debugEvent.dwThreadId << " exited with code: " << debugEvent.u.ExitThread.dwExitCode << "." << std::endl;
						//}

						auto proc = processMap[debugEvent.dwProcessId].get();
						proc->Threads.erase(proc->Threads.find(debugEvent.dwThreadId));
					}
					break;

					case EXIT_PROCESS_DEBUG_EVENT:
					{
						if (!quiet)
						{
							std::cout << "Process exited with code: " << debugEvent.u.ExitProcess.dwExitCode << "." << std::endl;
						}

						processMap.erase(debugEvent.dwProcessId);
						continueDebugging = processMap.empty() ? false : true;
					}
					break;

					case LOAD_DLL_DEBUG_EVENT:
					{
						auto name = GetFileNameFromHandle(debugEvent.u.LoadDll.hFile);
						if (!quiet)
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
							if (!quiet)
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
									if (!quiet)
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
											auto numberBytes = threadContextInfo.Eax;
											auto pointer = threadContextInfo.Ecx;
#endif

											auto data = new char[numberBytes+1];
											SIZE_T numberBytesRead;
											ReadProcessMemory(process->Handle, reinterpret_cast<LPVOID>(pointer), data, numberBytes, &numberBytesRead);
											data[numberBytesRead] = 0;

											if (!quiet)
											{
												std::cout << "Child process notification: " << data << std::endl;

												notifications.Handle(data, numberBytesRead);
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
												stack.AddrPC.Offset = threadContextInfo.Rip;    // EIP - Instruction Pointer
												stack.AddrPC.Mode = AddrModeFlat;
												stack.AddrFrame.Offset = threadContextInfo.Rsp; // ESP - Stack Pointer
												stack.AddrFrame.Mode = AddrModeFlat;
												stack.AddrStack.Offset = threadContextInfo.Rsp; // ESP - Stack Pointer (again!)
												stack.AddrStack.Mode = AddrModeFlat;
#else
												STACKFRAME64 stack = { 0 };
												stack.AddrPC.Offset = threadContextInfo.Eip;    // EIP - Instruction Pointer
												stack.AddrPC.Mode = AddrModeFlat;
												stack.AddrFrame.Offset = threadContextInfo.Ebp; // EBP
												stack.AddrFrame.Mode = AddrModeFlat;
												stack.AddrStack.Offset = threadContextInfo.Esp; // ESP - Stack Pointer
												stack.AddrStack.Mode = AddrModeFlat;
#endif

												// Let's initialize this once for our process.
												static SYMBOL_INFO* symbol = reinterpret_cast<SYMBOL_INFO *>(calloc(sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1));
												symbol->MaxNameLen = 255;
												symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

												BOOL status = S_OK;

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
														DWORD  dwDisplacement;
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
												}
												while (status);

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
			}
		}

		if (initializedDbgInfo)
		{
			for (auto& it : processMap)
			{
				SymCleanup(it.second->Handle);
			}
		}

		// Group profile data together:
		std::cout << "Gathering profile data of " << profileInfo.size() << " sources..." << std::endl;
		std::unordered_map<std::string, std::unique_ptr<std::vector<ProfileInfo>>> mergedInfo;

		float totalDeep = 0;
		for (auto& it : profileInfo)
		{
			for (auto& jt : it.second->lineHitCount)
			{
				totalDeep += jt.second.Deep;
			}
		}

		if (totalDeep == 0) { totalDeep = 1; }

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

			if (totalShallow == 0) { totalShallow = 1; }

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

		if (!quiet)
		{
			std::cout << "Filtering post-process notifications..." << std::endl;
		}

		coverageContext.Filter(notifications);

		if (!quiet)
		{
			std::cout << "Writing coverage report..." << std::flush;
		}

		auto outputFile = options.OutputFile;
		if (outputFile.size() == 0)
		{
			outputFile = options.Executable + ".cov";
		}

        // Write report of current execution
		coverageContext.WriteReport(options.ExportFormat, mergedInfo, outputFile);

		if (!quiet)
		{
			std::cout << "done." << std::endl;
		}
	}
};
