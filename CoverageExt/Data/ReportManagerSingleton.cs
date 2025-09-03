using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using EnvDTE;
using NubiloSoft.CoverageExt.Native;

namespace NubiloSoft.CoverageExt.Data
{
    /// <summary>
    /// Unfortunately we need a singleton because we cannot pass objects across the boundaries of DTE instances.
    /// </summary>
    public class ReportManagerSingleton
    {
        private static IReportManager instance = null;
        private static object lockObject = new object();

        public static IReportManager Instance(DTE dte)
        {
            if (dte != null)
            {
                lock (lockObject)
                {
                    if (instance == null || !instance.IsValid(Settings.Instance))
                    {
                        if (!Settings.Instance.UseOpenCppCoverageRunner)
                        {
                            switch (Settings.Instance.Format)
                            {
                                case CoverageFormat.Native:
                                instance = new Native.NativeReportManager(dte);
                                break;
                                case CoverageFormat.NativeV2:
                                instance = new Native.NativeV2ReportManager(dte);
                                break;
                            }
                        }
                        else
                        {
                            instance = new Cobertura.CoberturaReportManager(dte);
                        }
                    }
                }
            }

            return instance;
        }
    }
}
