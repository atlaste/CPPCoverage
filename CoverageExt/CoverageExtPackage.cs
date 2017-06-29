using System;
using System.Diagnostics;
using System.Globalization;
using System.Runtime.InteropServices;
using System.ComponentModel.Design;
using Microsoft.Win32;
using Microsoft.VisualStudio;
using Microsoft.VisualStudio.Shell.Interop;
using Microsoft.VisualStudio.OLE.Interop;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.VCProjectEngine;

namespace NubiloSoft.CoverageExt
{
    /// <summary>
    /// This is the class that implements the package exposed by this assembly.
    ///
    /// The minimum requirement for a class to be considered a valid package for Visual Studio
    /// is to implement the IVsPackage interface and register itself with the shell.
    /// This package uses the helper classes defined inside the Managed Package Framework (MPF)
    /// to do it: it derives from the Package class that provides the implementation of the 
    /// IVsPackage interface and uses the registration attributes defined in the framework to 
    /// register itself and its components with the shell.
    /// </summary>

    [PackageRegistration(UseManagedResourcesOnly = true)]
    [InstalledProductRegistration("#110", "#115", "1.0", IconResourceID = 400)]
    [ProvideMenuResource("Menus.ctmenu", 1)]
    [ProvideAutoLoad(UIContextGuids.SolutionHasSingleProject)]
    [ProvideToolWindow(typeof(Report.CoverageReportToolWindow))]
    [Guid(GuidList.guidCoverageExtPkgString)]
    public sealed class CoverageExtPackage : Package
    {
        /// <summary>
        /// Default constructor of the package.
        /// Inside this method you can place any initialization code that does not require 
        /// any Visual Studio service because at this point the package object is created but 
        /// not sited yet inside Visual Studio environment. The place to do all the other 
        /// initialization is the Initialize method.
        /// </summary>
        public CoverageExtPackage()
        {
            Debug.WriteLine(string.Format(CultureInfo.CurrentCulture, "Entering constructor for: {0}", this.ToString()));
        }

        private EnvDTE80.DTE2 dte;
        private DteInitializer dteInitializer;

        /// <summary>
        /// Initialization of the package; this method is called right after the package is sited, so this is the place
        /// where you can put all the initialization code that rely on services provided by VisualStudio.
        /// </summary>
        protected override void Initialize()
        {
            Debug.WriteLine(string.Format(CultureInfo.CurrentCulture, "Entering Initialize() of: {0}", this.ToString()));

            base.Initialize();
            InitializeDTE();

            // Add our command handlers for menu (commands must exist in the .vsct file)
            OleMenuCommandService mcs = GetService(typeof(IMenuCommandService)) as OleMenuCommandService;
            if (null != mcs)
            {
                // Create the command for the tool window
                CommandID toolwndCommandID = new CommandID(GuidList.guidCoverageExtCmdSet, (int)PkgCmdIDList.cmdCoverageReport);
                MenuCommand menuToolWin = new MenuCommand(ShowToolWindow, toolwndCommandID);
                mcs.AddCommand(menuToolWin);

                // Create the command for the context menu
                CommandID contextMenuCommandID = new CommandID(GuidList.guidProjectSpecificMenuCmdSet, (int)PkgCmdIDList.cmdCoverageGenerate);
                OleMenuCommand menuItem = new OleMenuCommand(ProjectContextMenuItemCallback, contextMenuCommandID);
                menuItem.BeforeQueryStatus += ProjectContextMenuItem_BeforeQueryStatus;
                mcs.AddCommand(menuItem);
            }
        }

        // See http://www.mztools.com/articles/2013/MZ2013029.aspx
        private void InitializeDTE()
        {
            IVsShell shellService;

            this.dte = this.GetService(typeof(Microsoft.VisualStudio.Shell.Interop.SDTE)) as EnvDTE80.DTE2;

            if (this.dte == null) // The IDE is not yet fully initialized
            {
                shellService = this.GetService(typeof(SVsShell)) as IVsShell;
                this.dteInitializer = new DteInitializer(shellService, this.InitializeDTE);
            }
            else
            {
                this.dteInitializer = null;
            }
        }

        /// <summary>
        /// This function is called when the user clicks the menu item that shows the 
        /// tool window. See the Initialize method to see how the menu item is associated to 
        /// this function using the OleMenuCommandService service and the MenuCommand class.
        /// </summary>
        private void ShowToolWindow(object sender, EventArgs e)
        {
            // Get the instance number 0 of this tool window. This window is single instance so this instance
            // is actually the only one.
            // The last flag is set to true so that if the tool window does not exists it will be created.
            ToolWindowPane window = this.FindToolWindow(typeof(Report.CoverageReportToolWindow), 0, true);
            if ((null == window) || (null == window.Frame))
            {
                throw new NotSupportedException(Resources.CanNotCreateWindow);
            }

            IVsWindowFrame windowFrame = (IVsWindowFrame)window.Frame;
            Microsoft.VisualStudio.ErrorHandler.ThrowOnFailure(windowFrame.Show());
        }

        /// <summary>
        /// Checks if we should render the context menu item or not.
        /// </summary>
        private void ProjectContextMenuItem_BeforeQueryStatus(object sender, EventArgs e)
        {
            OleMenuCommand menuCommand = sender as OleMenuCommand;
            if (menuCommand != null && dte != null)
            {
                menuCommand.Visible = false;  // default to not visible
                Array selectedProjects = (Array)dte.ActiveSolutionProjects;
                //only support 1 selected project
                if (selectedProjects.Length == 1)
                {
                    EnvDTE.Project project = (EnvDTE.Project)selectedProjects.GetValue(0);

                    // TODO FIXME: We should probably check if it's a DLL as well.

                    if (project.FullName.EndsWith(".vcxproj"))
                    {
                        menuCommand.Visible = true;
                    }
                }
            }
        }

        private void ProjectContextMenuItemCallback(object sender, EventArgs e)
        {
            var dte = this.dte.DTE;
            OutputWindow outputWindow = null;
            try
            {
                outputWindow = new OutputWindow(dte);

                OleMenuCommand menuCommand = sender as OleMenuCommand;
                if (menuCommand != null && dte != null)
                {
                    Array selectedProjects = (Array)dte.ActiveSolutionProjects;
                    //only support 1 selected project
                    if (selectedProjects.Length == 1)
                    {
                        EnvDTE.Project project = (EnvDTE.Project)selectedProjects.GetValue(0);
                        var vcproj = project.Object as VCProject;
                        if (vcproj != null)
                        {
                            IVCCollection configs = (IVCCollection)vcproj.Configurations;
                            VCConfiguration cfg = (VCConfiguration)vcproj.ActiveConfiguration;
                            VCPlatform currentPlatform = (VCPlatform)cfg.Platform;

                            string platform = currentPlatform == null ? null : currentPlatform.Name;
                            if (platform != null)
                            {
                                platform = platform.ToLower();
                                if (platform.Contains("x64"))
                                {
                                    platform = "x64";
                                }
                                else if (platform.Contains("x86") || platform.Contains("win32"))
                                {
                                    platform = "x86";
                                }
                                else
                                {
                                    throw new NotSupportedException("Platform is not supported.");
                                }
                            }
                            else
                            {
                                cfg = (VCConfiguration)configs.Item("Debug|x64");
                                platform = "x64";

                                if (cfg == null)
                                {
                                    throw new NotSupportedException("Cannot find x64 platform for project.");
                                }
                            }

                            var primaryOutput = cfg.PrimaryOutput;

                            if (primaryOutput != null)
                            {
                                var solutionFolder = System.IO.Path.GetDirectoryName(dte.Solution.FileName);

                                CoverageExecution executor = new CoverageExecution(dte, outputWindow);
                                executor.Start(
                                    solutionFolder,
                                    platform,
                                    System.IO.Path.GetDirectoryName(primaryOutput),
                                    System.IO.Path.GetFileName(primaryOutput));
                            }
                        }
                    }
                }
            }
            catch (NotSupportedException ex)
            {
                if (outputWindow != null)
                {
                    outputWindow.WriteLine("Error running coverage: {0}", ex.Message);
                }
            }
            catch (Exception ex)
            {
                if (outputWindow != null)
                {
                    outputWindow.WriteLine("Unexpected code coverage failure; error: {0}", ex.ToString());
                }
            }
        }
    }
}
