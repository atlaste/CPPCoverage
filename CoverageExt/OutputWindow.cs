using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using EnvDTE;
using EnvDTE80;

namespace NubiloSoft.CoverageExt
{
    public class OutputWindow
    {
        public OutputWindow(DTE dte)
        {
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
                lock (windowLock)
                {
                    window.OutputString(string.Format(format, par) + "\r\n");
                }
            }
        }
    }
}
