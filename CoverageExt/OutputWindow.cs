using EnvDTE;
using EnvDTE80;
using Microsoft.VisualStudio.Shell;

namespace NubiloSoft.CoverageExt
{
    public class OutputWindow
    {
        public OutputWindow(DTE dte)
        {
            Microsoft.VisualStudio.Shell.ThreadHelper.ThrowIfNotOnUIThread();
            try
            {
                lock (windowLock)
                {
                    var d = dte as DTE2;
                    if (d != null && d.ToolWindows != null && d.ToolWindows.OutputWindow != null)
                    {
                        var o = d.ToolWindows.OutputWindow;
                        for (int i = 1; i <= o.OutputWindowPanes.Count; ++i)
                        {
                            var wnd = o.OutputWindowPanes.Item(i);
                            if (wnd.Name == "CodeCoverage")
                            {
                                window = wnd;
                                break;
                            }
                        }

                        if (window == null)
                        {
                            window = o.OutputWindowPanes.Add("CodeCoverage");
                        }
                    }
                }
            }
            catch
            {
            }
        }

        private OutputWindowPane window;
        private static object windowLock = new object();

        public void Clear()
        {
            Microsoft.VisualStudio.Shell.ThreadHelper.ThrowIfNotOnUIThread();
            if (window != null)
            {
                lock (windowLock)
                {
                    window.Clear();
                    window.Activate();
                }
            }
        }

        public void WriteLine(string format, params object[] par)
        {
            ThreadHelper.ThrowIfNotOnUIThread();
            if (window != null)
            {
                string message = (par.Length != 0) ? string.Format(format, par) : format;
                lock (windowLock)
                {
                    window.OutputString(message + "\r\n");
                }
            }
        }

        public async System.Threading.Tasks.Task WriteLineAsync(string format, params object[] par) 
        {
            if (window != null)
            {
                string message = (par.Length != 0) ? string.Format(format, par) : format;
                await Microsoft.VisualStudio.Shell.ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();

                lock (windowLock)
                {
                    window.OutputString(message + "\r\n");
                }
            }
        }

        public async System.Threading.Tasks.Task WriteDebugLineAsync(string format, params object[] par)
        {
#if DEBUG
            await WriteLineAsync(format, par);
#endif
        }
    }
}
