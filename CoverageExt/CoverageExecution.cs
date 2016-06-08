using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NubiloSoft.CoverageExt
{
    public class CoverageExecution
    {
        public CoverageExecution(EnvDTE.DTE dte)
        {
            this.output = new OutputWindow(dte);
        }

        private StringBuilder sb = new StringBuilder();
        private StringBuilder tb = new StringBuilder();
        private DateTime lastEvent = DateTime.UtcNow;
        private OutputWindow output;

        public void Start(string solutionFolder, string dllFolder, string dllFilename)
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
            process.StartInfo.FileName = @"c:\Program Files\OpenCppCoverage\OpenCppCoverage.exe";

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

            StringBuilder argumentBuilder = new StringBuilder();
            argumentBuilder.Append("--quiet --export_type cobertura:\"");
            argumentBuilder.Append(resultFile);
            argumentBuilder.Append("\" --continue_after_cpp_exception --cover_children ");
            argumentBuilder.Append("--sources ");
            argumentBuilder.Append(sourcesFilter);
            argumentBuilder.Append(" -- ");
            argumentBuilder.Append(@"""C:\Program Files (x86)\Microsoft Visual Studio 14.0\Common7\IDE\CommonExtensions\Microsoft\TestWindow\vstest.console.exe""");
            argumentBuilder.Append(" /Platform:x64 \"");
            argumentBuilder.Append(Path.Combine(dllFolder, dllFilename));
            argumentBuilder.Append("\"");

            process.StartInfo.Arguments = argumentBuilder.ToString();
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
            while (!exited && lastEvent.AddMinutes(1) > DateTime.UtcNow)
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

            // All fine, update file:
            if (File.Exists(defResultFile))
            {
                File.Delete(defResultFile);
            }
            File.Move(resultFile, defResultFile);
        }

        private void OutputHandler(object sender, DataReceivedEventArgs e)
        {
            // Redirect to output window
            if (e.Data != null)
            {
                string s = e.Data;

                output.WriteLine(s);
                tb.AppendLine(s);

                lastEvent = DateTime.UtcNow;
            }
        }
    }
}
