namespace NubiloSoft.CoverageExt.Data
{
    public class ReportManagerSingleton
    {
        private static IReportManager instance = null;
        private static readonly object lockObject = new object();

        public static IReportManager Instance(string solutionFolder)
        {
            if (solutionFolder != null)
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
                                    instance = new Native.NativeReportManager(solutionFolder);
                                    break;
                                case CoverageFormat.NativeV2:
                                    instance = new Native.NativeV2ReportManager(solutionFolder);
                                    break;
                                case CoverageFormat.Cobertura:
                                    instance = new Cobertura.CoberturaReportManager(solutionFolder);
                                    break;
                            }
                        }
                        else
                        {
                            instance = new Cobertura.CoberturaReportManager(solutionFolder);
                        }
                    }
                }
            }

            return instance;
        }
    }
}
