using NubiloSoft.CoverageExt.Data;
using NubiloSoft.CoverageExt.Loggers;
using System;

namespace NubiloSoft.CoverageExt.Native
{
    public class NativeV2ReportManager : NativeReportManager
    {
        public NativeV2ReportManager(string solutionFolder) : base(solutionFolder)
        { }

        public override ICoverageData Load(string filename)
        {
            ICoverageData report = null;
            if (filename != null)
            {
                try
                {
                    report = new NativeV2Data();
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
