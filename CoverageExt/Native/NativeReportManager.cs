using System;
using System.IO;
using EnvDTE;
using NubiloSoft.CoverageExt.Data;

namespace NubiloSoft.CoverageExt.Native
{
    public class NativeReportManager : Data.IReportManager
    {
        public NativeReportManager(DTE dte)
        {
            this.dte = dte;
            this.output = new OutputWindow(dte);

            this.activeCoverageReport = null;
            this.activeCoverageFilename = null;
        }

        private DTE dte;
        private OutputWindow output;

        private Data.ICoverageData activeCoverageReport;
        private string activeCoverageFilename;

        private object lockObject = new object();

        public ICoverageData UpdateData()
        {
            // It makes no sense to have multiple instances of our coverage data in our memory, so
            // this is exposed as a singleton. Updating needs concurrency control. It's pretty fast, so 
            // a simple lock will do.
            //
            // We update as all-or-nothing, and use the result from UpdateData. This means no other concurrency
            // control is required; there are no conflicts.
            lock (lockObject)
            {
                return UpdateDataImpl();
            }
        }

        public void ResetData()
        {
            lock (lockObject)
            {
                this.activeCoverageReport = null;
            }
        }

        private ICoverageData UpdateDataImpl()
        {
            try
            {
                string filename = dte.Solution.FileName;
                string folder = System.IO.Path.GetDirectoryName(filename);
                string coverageFile = System.IO.Path.Combine(folder, "CodeCoverage.cov");

                if (activeCoverageFilename != coverageFile)
                {
                    activeCoverageFilename = coverageFile;
                    activeCoverageReport = null;
                }

                if (File.Exists(coverageFile))
                {
                    if (activeCoverageReport != null)
                    {
                        var lastWT = new FileInfo(coverageFile).LastWriteTimeUtc;
                        if (lastWT > activeCoverageReport.FileDate)
                        {
                            activeCoverageReport = null;
                        }
                    }

                    if (activeCoverageReport == null)
                    {
                        output.WriteLine("Updating coverage results from: {0}", coverageFile);
                        activeCoverageReport = Load(coverageFile);
                        activeCoverageFilename = coverageFile;
                    }
                }
            }
            catch { }

            return activeCoverageReport;
        }

        private ICoverageData Load(string filename)
        {
            ICoverageData report = null;
            if (filename != null)
            {
                try
                {
                    report = new Native.NativeData(filename);
                }
                catch (Exception e)
                {
                    output.WriteLine("Error loading coverage report: {0}", e.Message);
                }
            }
            return report;
        }
    }
}
