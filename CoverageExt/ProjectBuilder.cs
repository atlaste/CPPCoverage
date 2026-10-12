using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Threading;
using NubiloSoft.CoverageExt.Loggers;
using System;
using System.Threading.Tasks;

namespace NubiloSoft.CoverageExt
{
    public class ProjectBuilder
    {
        private readonly EnvDTE.DTE dte;

        private readonly string projectName;
        private readonly string configName;

        private readonly AsyncManualResetEvent buildCompleted = new AsyncManualResetEvent();
        private bool buildSucceeded = false;

        public ProjectBuilder(EnvDTE.DTE dte, string projectName, string configName)
        {
            this.dte = dte;
            this.projectName = projectName;
            this.configName = configName;
        }

        public async Task<bool> BuildAsync()
        {
            await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();

            var buildEvents = dte.Events.BuildEvents;
            buildEvents.OnBuildProjConfigDone += OnBuildDone;

            try
            {
                Logger.Info($"Start building the {projectName} project in {configName} configuration");
                dte.Solution.SolutionBuild.BuildProject(configName, projectName, false);
                await buildCompleted.WaitAsync();
                return buildSucceeded;
            }
            finally
            {
                buildEvents.OnBuildProjConfigDone -= OnBuildDone;
            }
        }

        private void OnBuildDone(string project, string projectConfig, string platform, string solutionConfig, bool success)
        {
            if (project != projectName || solutionConfig != configName) return;
            Logger.Info($"Build completed, result: {success}");

            buildSucceeded = success;
            buildCompleted.Set();
        }
    }
}
