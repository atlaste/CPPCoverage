using System;
using System.IO;
using EnvDTE;
using NubiloSoft.CoverageExt.Data;

namespace NubiloSoft.CoverageExt.Native
{
    public class NativeV2ReportManager : NativeReportManager
    {
        public NativeV2ReportManager(DTE dte) : base(dte)
        {}

        public override bool IsValid(Settings instance)
        {
            return !instance.UseOpenCppCoverageRunner
              && instance.Format == CoverageFormat.NativeV2;
        }

        public override ICoverageData Load(string filename, string solution)
        {
            Microsoft.VisualStudio.Shell.ThreadHelper.ThrowIfNotOnUIThread();

            ICoverageData report = null;
            if (filename != null)
            {
                try
                {
                    report = new Native.NativeV2Data();
                    report.Parsing(filename, solution);
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
