using NubiloSoft.CoverageExt.Data;
using NubiloSoft.CoverageExt.Loggers;
using System;
using System.IO;

namespace NubiloSoft.CoverageExt.Cobertura
{
    public class CoberturaReportManager : IReportManager
    {
        public CoberturaReportManager(string solutionFolder)
        {
            this.solutionFolder = solutionFolder;

            activeCoverageReport = null;
            activeCoverageFilename = null;
        }

        private readonly string solutionFolder;

        private Data.ICoverageData activeCoverageReport;
        private string activeCoverageFilename;

        private readonly object lockObject = new object();

        public bool IsValid(Settings instance)
        {
            return instance.Format == CoverageFormat.Cobertura;
        }

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
                string coverageFile = Path.Combine(solutionFolder, "CodeCoverage.xml");

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
                        Logger.Info("Updating coverage results from: {0}", coverageFile);
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
                    report = new CoberturaData();
                    report.Parsing(filename);
                }
                catch (Exception e)
                {
                    Logger.Info("Error loading coverage report: {0}", e.Message);
                }
            }
            return report;
        }
    }
}
