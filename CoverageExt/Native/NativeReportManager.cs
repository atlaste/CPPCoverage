using EnvDTE;
using NubiloSoft.CoverageExt.Data;
using System;
using System.IO;

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

        protected DTE dte;
        protected OutputWindow output;

        protected Data.ICoverageData activeCoverageReport;
        protected string activeCoverageFilename;

        protected object lockObject = new object();

        public virtual bool IsValid(Settings instance)
        {
            return !instance.UseOpenCppCoverageRunner
              && instance.Format == CoverageFormat.Native;
        }

        ICoverageData Data.IReportManager.UpdateData()
        {
            Microsoft.VisualStudio.Shell.ThreadHelper.ThrowIfNotOnUIThread();
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
            Microsoft.VisualStudio.Shell.ThreadHelper.ThrowIfNotOnUIThread();
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
                        output.WriteLine("------ Start update Coverage ------");
                        output.WriteLine("Updating coverage results from: {0}", coverageFile);
                        var watch = System.Diagnostics.Stopwatch.StartNew();
                        activeCoverageReport = Load(coverageFile);
                        activeCoverageFilename = coverageFile;
                        watch.Stop();
                        output.WriteLine("========== Done in {0} ms with {1} entries ==========", watch.ElapsedMilliseconds, activeCoverageReport.nbEntries());
                    }
                }
            }
            catch { }

            return activeCoverageReport;
        }

        public Data.ICoverageData UpdateData()
        {
            throw new NotImplementedException();
        }

        virtual public ICoverageData Load(string filename)
        {
            Microsoft.VisualStudio.Shell.ThreadHelper.ThrowIfNotOnUIThread();

            ICoverageData report = null;
            if (filename != null)
            {
                try
                {
                    report = new Native.NativeData();
                    report.Parsing(filename);
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
