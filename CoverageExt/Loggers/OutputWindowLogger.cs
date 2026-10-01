using EnvDTE;
using EnvDTE80;
using Microsoft.VisualStudio.Shell;
using System;
using System.Collections.Concurrent;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace NubiloSoft.CoverageExt.Loggers
{
    internal sealed class OutputWindowLogger
    {
        private const string PaneName = "CodeCoverage";
        private readonly OutputWindowPane window;
        private readonly ConcurrentQueue<string> queue = new ConcurrentQueue<string>();
        private int flushScheduled;

        public OutputWindowLogger(DTE dte)
        {
            ThreadHelper.ThrowIfNotOnUIThread();

            if (!(dte is DTE2 dte2))
                return;

            var outputWindow = dte2.ToolWindows?.OutputWindow;

            if (outputWindow == null)
                return;

            for (int i = 1; i <= outputWindow.OutputWindowPanes.Count; i++)
            {
                var pane = outputWindow.OutputWindowPanes.Item(i);

                if (pane.Name == PaneName)
                {
                    window = pane;
                    break;
                }
            }

            if (window == null)
            {
                window = outputWindow.OutputWindowPanes.Add(PaneName);
            }
        }

        public void WriteLine(string format, params object[] args)
        {
            string message = args.Length == 0 ? format : string.Format(format, args);

            queue.Enqueue(message + Environment.NewLine);
            ScheduleFlush();
        }

        public void Clear()
        {
            ThreadHelper.ThrowIfNotOnUIThread();

            while (queue.TryDequeue(out _))
            {
            }

            window?.Clear();
            window?.Activate();
        }

        private void ScheduleFlush()
        {
            if (Interlocked.Exchange(ref flushScheduled, 1) != 0)
                return;

            _ = FlushAsync();
        }

        private async Task FlushAsync()
        {
            try
            {
                await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();
                Flush();
            }
            catch (Exception ex)
            {
                System.Diagnostics.Debug.WriteLine($"OutputWindowLogger flush failed: {ex}");
            }
            finally
            {
                Interlocked.Exchange(ref flushScheduled, 0);

                if (!queue.IsEmpty)
                {
                    ScheduleFlush();
                }
            }
        }

        private void Flush()
        {
            ThreadHelper.ThrowIfNotOnUIThread();

            if (window == null)
            {
                while (queue.TryDequeue(out _)) ;
                return;
            }

            if (queue.IsEmpty)
                return;

            var builder = new StringBuilder();

            while (queue.TryDequeue(out string message))
            {
                builder.Append(message);
            }

            if (builder.Length > 0)
            {
                window.OutputString(builder.ToString());
            }
        }
    }
}