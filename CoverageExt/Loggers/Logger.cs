using EnvDTE;
using Microsoft.VisualStudio.Shell;

namespace NubiloSoft.CoverageExt.Loggers
{
    public static class Logger
    {
        private static OutputWindowLogger instance;

        public static void Initialize(DTE dte)
        {
            ThreadHelper.ThrowIfNotOnUIThread();
            if (instance != null)
                return;

            instance = new OutputWindowLogger(dte);
        }
        public static void Info(string format, params object[] args)
        {
            instance?.WriteLine(format, args);
        }

        public static void Debug(string format, params object[] args)
        {
#if DEBUG
            Info(format, args);
#endif
        }
        public static void Clear()
        {
            ThreadHelper.ThrowIfNotOnUIThread();
            instance?.Clear();
        }
    }
}
