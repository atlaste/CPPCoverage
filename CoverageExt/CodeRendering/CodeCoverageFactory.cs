using System;
using System.ComponentModel.Composition;
using EnvDTE;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Editor;
using Microsoft.VisualStudio.Utilities;
using DTE = EnvDTE.DTE;


namespace NubiloSoft.CoverageExt.CodeRendering
{
    /// <summary>
    /// Establishes an <see cref="IAdornmentLayer"/> to place the adornment on and exports the <see cref="IWpfTextViewCreationListener"/>
    /// that instantiates the adornment on the event of a <see cref="IWpfTextView"/>'s creation
    /// </summary>
    [Export(typeof(IWpfTextViewCreationListener))]
    [ContentType("text")]
    [TextViewRole(PredefinedTextViewRoles.Document)]
    public sealed class CodeCoverageFactory : IWpfTextViewCreationListener
    {
        /// <summary>
        /// Defines the adornment layer for the adornment. This layer is ordered 
        /// after the selection layer in the Z-order
        /// </summary>
        [Export(typeof(AdornmentLayerDefinition))]
        [Name("CodeCoverage")]
        [Order(After = PredefinedAdornmentLayers.BraceCompletion, Before = PredefinedAdornmentLayers.Selection)]
        public AdornmentLayerDefinition editorAdornmentLayer = null;

        [Import]
        public SVsServiceProvider ServiceProvider = null;

        [Import]
        public ITextDocumentFactoryService textDocumentFactory { get; set; }

        /// <summary>
        /// Instantiates a CodeCoverage manager when a textView is created.
        /// </summary>
        /// <param name="textView">The <see cref="IWpfTextView"/> upon which the adornment should be placed</param>
        public void TextViewCreated(IWpfTextView textView)
        {
            Microsoft.VisualStudio.Shell.ThreadHelper.ThrowIfNotOnUIThread();

            DTE dte = (DTE)ServiceProvider.GetService(typeof(DTE));

            // Store this thing somewhere so our GC doesn't incidentally destroy it.
            var cover = new CodeCoverage(textView, dte, textDocumentFactory);

            textView.Closed += (object sender, EventArgs e) => {
                cover.Close();
            };
        }
    }
}
