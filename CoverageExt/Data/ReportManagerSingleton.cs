namespace NubiloSoft.CoverageExt.Data
{
    public class ReportManagerSingleton
    {
        private static IReportManager instance = null;
        private static string solutionFolder = null;
        private static CoverageFormat format = CoverageFormat.Native;
        private static bool isSettingsInit = false;
        private static readonly object lockObject = new object();

        private static void CreateInstance()
        {
            instance = null;

            if (solutionFolder == null) return;
            if (!isSettingsInit) return;

            switch (format)
            {
                default:
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

        public static IReportManager Instance()
        {
            lock (lockObject)
            {
                return instance;
            }
        }

        public static void OnLoadedSolution(string folder)
        {
            lock (lockObject)
            {
                // Nothing changed
                if (solutionFolder == folder) return;

                solutionFolder = folder;
                CreateInstance();
            }
        }

        private static CoverageFormat FormatFromSettings()
        {
            if (Settings.Instance.UseOpenCppCoverageRunner) return CoverageFormat.Cobertura;
            return Settings.Instance.Format;
        }

        public static void OnChangedSettings()
        {
            CoverageFormat newFormat = FormatFromSettings();

            lock (lockObject)
            {
                // create the object if the settings are being loaded for the first time or when the format changes.
                if (isSettingsInit && (format == newFormat)) return;

                isSettingsInit = true;
                format = newFormat;
                CreateInstance();
            }
        }
    }
}
