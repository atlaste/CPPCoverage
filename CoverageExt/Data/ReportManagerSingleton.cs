using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using EnvDTE;

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
            if (dte != null && instance == null)
            {
                lock (lockObject)
                {
                    if (instance == null)
                    {
                        if (Settings.Instance.UseNativeCoverageSupport)
                        {
                            instance = new Native.NativeReportManager(dte);
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
