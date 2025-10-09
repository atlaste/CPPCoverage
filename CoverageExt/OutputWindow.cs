using EnvDTE;
using EnvDTE80;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Threading;
using System.Windows.Forms;

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
                    if (dte is DTE2 d && d.ToolWindows != null && d.ToolWindows.OutputWindow != null)
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

        private readonly OutputWindowPane window;
        private static readonly object windowLock = new object();

        public void Clear()
        {
            ThreadHelper.ThrowIfNotOnUIThread();
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
            if (window != null)
            {
                string message = (par.Length != 0) ? string.Format(format, par) : format;
                lock (windowLock)
                {
                    _ = WriteLineAsync(message);
                }
            }
        }

        private async System.Threading.Tasks.Task WriteLineAsync(string message) 
        {
            await Microsoft.VisualStudio.Shell.ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();

            lock (windowLock)
            {
                window.OutputString(message + "\r\n");
            }
        }

        public void WriteDebugLine(string format, params object[] par)
        {
#if DEBUG
            WriteLine(format, par);
#endif
        }
    }
}
