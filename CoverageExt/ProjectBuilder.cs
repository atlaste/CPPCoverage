using System;

namespace NubiloSoft.CoverageExt
{
    public class ProjectBuilder
    {
        private readonly EnvDTE.DTE dte;
        private readonly OutputWindow outputWindow;

        private readonly string projectName;
        private readonly string configName;
        private readonly Action onSuccessAction;

        public ProjectBuilder( EnvDTE.DTE dte, OutputWindow outputWindow, string projectName, string configName, Action onSuccessAction )
        {
            this.dte = dte;
            this.outputWindow = outputWindow;
            this.projectName = projectName;
            this.configName = configName;
            this.onSuccessAction = onSuccessAction;
        }

        public void Build()
        {
            Microsoft.VisualStudio.Shell.ThreadHelper.ThrowIfNotOnUIThread();

            dte.Events.BuildEvents.OnBuildProjConfigDone += OnBuildDone;
            outputWindow.WriteLine("Start building the {0} project in {1} configuration", projectName, configName);
            dte.Solution.SolutionBuild.BuildProject(configName, projectName, false);
        }

        private void OnBuildDone( string project, string projectConfig, string platform, string solutionConfig, bool success )
        {
            Microsoft.VisualStudio.Shell.ThreadHelper.ThrowIfNotOnUIThread();

            if (project == projectName && solutionConfig == configName)
            {
                outputWindow.WriteLine("Build completed, result: {0}", success.ToString());
                dte.Events.BuildEvents.OnBuildProjConfigDone -= OnBuildDone;
                if (success)
                {
                    onSuccessAction();
                }
            }
        }
    }
}
