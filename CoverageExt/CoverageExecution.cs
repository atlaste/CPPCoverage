using System;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Threading;
using DTE = EnvDTE.DTE;

namespace NubiloSoft.CoverageExt
{
    public class CoverageExecution
    {
        public CoverageExecution(DTE dte, OutputWindow output)
        {
            this.dte = dte;
            this.output = output;
        }

        private static string ProgramFilesX86Path = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86);
        private static string ProgramFilesX64Path = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);

        private StringBuilder tb = new StringBuilder();
        private DateTime lastEvent = DateTime.UtcNow;

        private DTE dte;
        private OutputWindow output;

        private int running = 0;

        public void Start(string solutionFolder, string platform, string dllFolder, string dllFilename, string workingDirectory, string commandline)
        {
            // We want 1 thread to do this; never more.
            if (Interlocked.CompareExchange(ref running, 1, 0) == 0)
            {
                Thread t = new Thread(() => StartImpl(solutionFolder, platform, dllFolder, dllFilename, workingDirectory, commandline))
                {
                    IsBackground = true,
                    Name = "Code coverage generator thread"
                };

                this.output.Clear();
                this.output.WriteLine("Calculating code coverage...");

                t.Start();
            }
        }

        private string CoverageExePath( string platform )
        {
            if (Settings.Instance.UseNativeCoverageSupport)
            {
                // Find the executables for Coverage.exe
                string location = typeof(CoverageExecution).Assembly.Location;
                string folder = Path.GetDirectoryName(location);
                return (platform == "x86") ?
                    Path.Combine(folder, "Resources\\Coverage-x86.exe") :
                    Path.Combine(folder, "Resources\\Coverage-x64.exe");
            }
            else
            {
                if (platform == "x86")
                {
                    string path1 = Path.Combine(ProgramFilesX64Path, "OpenCppCoverage\\x86\\OpenCppCoverage.exe");
                    string path2 = Path.Combine(ProgramFilesX86Path, "OpenCppCoverage\\OpenCppCoverage.exe");

                    return File.Exists(path1) ? path1 : path2;
                }
                else
                {
                    return Path.Combine(ProgramFilesX64Path, "OpenCppCoverage\\OpenCppCoverage.exe");
                }
            }
        }

        private string CreateVsTestExePath()
        {
            // TODO: We can do much better here by using the registry...
            var folders = new[]
            {
                @"\Microsoft Visual Studio\2022\Professional\Common7\IDE\Extensions\TestPlatform\",
                @"\Microsoft Visual Studio\2022\Community\Common7\IDE\Extensions\TestPlatform\",
                @"\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\Extensions\TestPlatform\",
                @"\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\TestWindow\",
                @"\Microsoft Visual Studio\2019\Community\Common7\IDE\CommonExtensions\Microsoft\TestWindow\",
                @"\Microsoft Visual Studio\2019\Professional\Common7\IDE\CommonExtensions\Microsoft\TestWindow\",
                @"\Microsoft Visual Studio\2019\Enterprise\Common7\IDE\CommonExtensions\Microsoft\TestWindow\",
                @"\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\TestWindow\",
                @"\Microsoft Visual Studio\2017\Community\Common7\IDE\CommonExtensions\Microsoft\TestWindow\",
                @"\Microsoft Visual Studio\2017\Professional\Common7\IDE\CommonExtensions\Microsoft\TestWindow\",
                @"\Microsoft Visual Studio\2017\Enterprise\Common7\IDE\CommonExtensions\Microsoft\TestWindow\",
                @"\Microsoft Visual Studio\2017\BuildTools\Common7\IDE\CommonExtensions\Microsoft\TestWindow\",
                @"\Microsoft Visual Studio 19.0\Common7\IDE\CommonExtensions\Microsoft\TestWindow\",
                @"\Microsoft Visual Studio 16.0\Common7\IDE\CommonExtensions\Microsoft\TestWindow\",
                @"\Microsoft Visual Studio 15.0\Common7\IDE\CommonExtensions\Microsoft\TestWindow\",
                @"\Microsoft Visual Studio 14.0\Common7\IDE\CommonExtensions\Microsoft\TestWindow\",
                @"\Microsoft Visual Studio 13.0\Common7\IDE\CommonExtensions\Microsoft\TestWindow\"
            };
            var pf = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86).TrimEnd('\\');

            foreach (var fold in folders)
            {
                string file = pf + fold + "vstest.console.exe";
                if (File.Exists(file))
                {
                    this.output.WriteLine("Found vstest console app.");
                    return file;
                }
            }

            pf = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles).TrimEnd('\\');
            this.output.WriteLine(pf);

            foreach (var fold in folders)
            {
                string file = pf + fold + "vstest.console.exe";
                this.output.WriteLine(file);
                if (File.Exists(file))
                {
                    this.output.WriteLine("Found vstest console app.");
                    return file;
                }
            }

            throw new Exception("Cannot find vstest.console.exe.");
        }

        private string PrepareArgument( string solutionFolder, string platform, string dllFolder, string dllFilename, string workingDirectory, string commandline, string resultFile )
        {
            StringBuilder argumentBuilder = new StringBuilder();
            if (Settings.Instance.UseNativeCoverageSupport)
            {
                argumentBuilder.Append("-o \"");
                argumentBuilder.Append(resultFile);
                argumentBuilder.Append("\" -p \"");
                argumentBuilder.Append(solutionFolder.TrimEnd('\\', '/'));

                if (workingDirectory != null && workingDirectory.Length > 0)
                {
                    // When directory finish by \ : c++ read \" and arguments is badly computed !
                    workingDirectory = workingDirectory.TrimEnd('\\', '/');

                    argumentBuilder.Append("\" -w \"");
                    argumentBuilder.Append(workingDirectory);

                }
                else
                {
                    argumentBuilder.Append("\"");
                }

                if (dllFilename.EndsWith(".exe", StringComparison.InvariantCultureIgnoreCase))
                {
                    argumentBuilder.Append("\" -- \"");
                }
                else
                {
                    string vsTestExe = CreateVsTestExePath();

                    argumentBuilder.Append("\" -- ");
                    argumentBuilder.Append(@"""" + vsTestExe + @"""");
                    argumentBuilder.Append(" /Platform:" + platform + " \"");
                }

                argumentBuilder.Append(Path.Combine(dllFolder, dllFilename));
                if (commandline != null && commandline.Length > 0)
                {
                    argumentBuilder.Append("\" ");
                    argumentBuilder.Append(commandline);
                }
                else
                {
                    argumentBuilder.Append("\"");
                }
            }
            else
            {
                string sourcesFilter = solutionFolder;
                int smidx = sourcesFilter.IndexOf(' ');
                if (smidx > 0)
                {
                    sourcesFilter = sourcesFilter.Substring(0, smidx);
                    int lidx = sourcesFilter.LastIndexOf('\\');
                    if (lidx >= 0)
                    {
                        sourcesFilter = sourcesFilter.Substring(0, lidx - 1);
                    }
                }

                if (dllFilename.EndsWith(".exe", StringComparison.InvariantCultureIgnoreCase))
                {
                    argumentBuilder.Append("--quiet --export_type cobertura:\"");
                    argumentBuilder.Append(resultFile);
                    argumentBuilder.Append("\" --continue_after_cpp_exception --cover_children ");
                    argumentBuilder.Append("--sources ");
                    argumentBuilder.Append(sourcesFilter);
                    argumentBuilder.Append(" -- \"");
                    argumentBuilder.Append(Path.Combine(dllFolder, dllFilename));
                    argumentBuilder.Append("\"");
                }
                else
                {
                    argumentBuilder.Append("--quiet --export_type cobertura:\"");
                    argumentBuilder.Append(resultFile);
                    argumentBuilder.Append("\" --continue_after_cpp_exception --cover_children ");
                    argumentBuilder.Append("--sources ");
                    argumentBuilder.Append(sourcesFilter);
                    argumentBuilder.Append(" -- ");

                    string vsTestExe = CreateVsTestExePath();
                    argumentBuilder.Append(@"""" + vsTestExe + @"""");
                    argumentBuilder.Append(" /Platform:" + platform + " \"");
                    argumentBuilder.Append(Path.Combine(dllFolder, dllFilename));
                    argumentBuilder.Append("\"");
                }
            }
            return argumentBuilder.ToString();
        }

        private void StartImpl(string solutionFolder, string platform, string dllFolder, string dllFilename, string workingDirectory, string commandline)
        {
            try
            {
                if (Settings.Instance.UseNativeCoverageSupport)
                {
                    // Delete old coverage file
                    string resultFile = Path.Combine(solutionFolder, "CodeCoverage.tmp.cov");
                    string defResultFile = Path.Combine(solutionFolder, "CodeCoverage.cov");
                    if (File.Exists(resultFile))
                    {
                        File.Delete(resultFile);
                    }

                    // Create your Process
                    Process process = new Process();
                    process.StartInfo.FileName = CoverageExePath(platform);

                    if (!File.Exists(process.StartInfo.FileName))
                    {
                        throw new NotSupportedException("Coverage.exe instance for platform was not found. Expected: " + process.StartInfo.FileName);
                    }

                    string argument = PrepareArgument(solutionFolder, platform, dllFolder, dllFilename, workingDirectory, commandline, resultFile);

#if DEBUG
                    this.output.WriteLine("Execute coverage: {0}", argument);
#endif

                    process.StartInfo.WorkingDirectory = dllFolder;
                    process.StartInfo.Arguments = argument;
                    process.StartInfo.CreateNoWindow = true;
                    process.StartInfo.UseShellExecute = false;
                    process.StartInfo.RedirectStandardOutput = true;
                    process.StartInfo.RedirectStandardError = true;

                    process.OutputDataReceived += new DataReceivedEventHandler(OutputHandler);
                    process.ErrorDataReceived += new DataReceivedEventHandler(OutputHandler);

                    process.Start();

                    process.BeginOutputReadLine();
                    process.BeginErrorReadLine();

                    bool exited = false;
                    while (!exited && lastEvent.AddMinutes(15) > DateTime.UtcNow)
                    {
                        exited = process.WaitForExit(1000 * 60);
                    }

                    if (!exited)
                    {
                        // Kill process.
                        process.Kill();
                        throw new Exception("Creating code coverage timed out (more than 15min).");
                    }

                    string output = tb.ToString();

                    if (output.Contains("Usage:"))
                    {
                        throw new Exception("Incorrect command line argument passed to coverage tool");
                    }

                    if (output.Contains("Error: the test source file"))
                    {
                        throw new Exception("Cannot find test source file " + dllFilename);
                    }

                    if (File.Exists(resultFile))
                    {
                        // All fine, update file:
                        if (File.Exists(defResultFile))
                        {
                            File.Delete(defResultFile);
                        }
                        File.Move(resultFile, defResultFile);
                    }
                    else
                    {
                        this.output.WriteLine("No coverage report generated. Cannot continue.");
                    }
                }
                else
                {
                    // Delete old coverage file
                    string resultFile = Path.Combine(solutionFolder, "CodeCoverage.tmp.xml");
                    string defResultFile = Path.Combine(solutionFolder, "CodeCoverage.xml");
                    if (File.Exists(resultFile))
                    {
                        File.Delete(resultFile);
                    }

                    // Create your Process
                    Process process = new Process();
                    process.StartInfo.FileName = CoverageExePath(platform);

                    if (!File.Exists(process.StartInfo.FileName))
                    {
                        throw new NotSupportedException("OpenCPPCoverage was not found. Expected: " + process.StartInfo.FileName);
                    }

                    string argument = PrepareArgument(solutionFolder, platform, dllFolder, dllFilename, workingDirectory, commandline, resultFile);

#if DEBUG
                    this.output.WriteLine("Execute coverage: {0}", argument);
#endif

                    process.StartInfo.WorkingDirectory = Path.GetDirectoryName(resultFile);
                    process.StartInfo.Arguments = argument;
                    process.StartInfo.CreateNoWindow = true;
                    process.StartInfo.UseShellExecute = false;
                    process.StartInfo.RedirectStandardOutput = true;
                    process.StartInfo.RedirectStandardError = true;

                    process.OutputDataReceived += new DataReceivedEventHandler(OutputHandler);
                    process.ErrorDataReceived += new DataReceivedEventHandler(OutputHandler);

                    process.Start();

                    process.BeginOutputReadLine();
                    process.BeginErrorReadLine();

                    bool exited = false;
                    while (!exited && lastEvent.AddMinutes(15) > DateTime.UtcNow)
                    {
                        exited = process.WaitForExit(1000 * 60);
                    }

                    if (!exited)
                    {
                        // Kill process.
                        process.Kill();
                        throw new Exception("Creating code coverage timed out.");
                    }

                    string output = tb.ToString();

                    if (output.Contains("Usage:"))
                    {
                        throw new Exception("Incorrect command line argument passed to coverage tool");
                    }

                    if (output.Contains("Error: the test source file"))
                    {
                        throw new Exception("Cannot find test source file " + dllFilename);
                    }

                    if (File.Exists(resultFile))
                    {
                        // All fine, update file:
                        if (File.Exists(defResultFile))
                        {
                            File.Delete(defResultFile);
                        }
                        File.Move(resultFile, defResultFile);
                    }
                    else
                    {
                        this.output.WriteLine("No coverage report generated. Cannot continue.");
                    }

                }

            }
            catch (Exception ex)
            {
                output.WriteLine("Uncaught error during coverage execution: {0}", ex.Message);
            }
            Data.ReportManagerSingleton.Instance(dte).ResetData();
            Interlocked.Exchange(ref running, 0);
        }

        private void OutputHandler(object sender, DataReceivedEventArgs e)
        {
            // Redirect to output window
            if (!String.IsNullOrEmpty(e.Data))
            {
                string s = e.Data;

                output.WriteLine(s);
                tb.AppendLine(s);

                lastEvent = DateTime.UtcNow;
            }
        }
    }
}
