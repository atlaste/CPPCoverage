using Microsoft.VisualStudio.PlatformUI;
using Microsoft.VisualStudio.Shell;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Editor;
using Microsoft.VisualStudio.Text.Formatting;
using NubiloSoft.CoverageExt.Data;
using System;
using System.Collections.Generic;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;

namespace NubiloSoft.CoverageExt.CodeRendering
{
    public class CodeCoverage : IDisposable
    {
        internal IAdornmentLayer layer;
        internal IWpfTextView view;

        internal SolidColorBrush uncoveredBrush;
        internal SolidColorBrush uncoveredPenBrush;
        internal Pen uncoveredPen;
        internal SolidColorBrush coveredBrush;
        internal SolidColorBrush coveredPenBrush;
        internal SolidColorBrush classicPenBrush;
        internal Pen coveredPen;

        internal SolidColorBrush partialCoveredBrush;
        internal SolidColorBrush partialCoveredPenBrush;
        internal Pen partialCoveredPen;

        private readonly EnvDTE.DTE dte;
        private readonly OutputWindow outputWindow;

        private readonly ITextDocumentFactoryService textDocumentFactory;
        private ITextDocument TextDocument;
        private DateTime currentReportDate;
        private ProfileVector currentProfile;

        private IFileCoverageData activeReport;

        public CodeCoverage(IWpfTextView view, EnvDTE.DTE dte, ITextDocumentFactoryService textDocumentFactory)
        {
            this.dte = dte;
            this.outputWindow = new OutputWindow(dte);
            this.view = view;
            this.textDocumentFactory = textDocumentFactory;
            this.layer = view.GetAdornmentLayer("CodeCoverage");
            //this.layer.Opacity = 0.4;
            currentReportDate = DateTime.MinValue;

            SetupHandleEvents(true);

            // make sure the brushes are at least initialized once
            InitializeColors();
        }

        private void SetupHandleEvents(bool setup)
        {
            if (setup)
            {
                // listen to events that change the setting properties
                Settings.Instance.OnShowCodeCoveragePropertyChanged += Instance_OnShowCodeCoveragePropertyChanged;
                Settings.Instance.OnSettingsChanged += Instance_OnSettingsChanged;
                Settings.Instance.RedrawNeeded += Instance_OnRedrawNeeded;
                Settings.Instance.CleanNeeded += Instance_OnClean;
                VSColorTheme.ThemeChanged += HandleThemeChange; // When VS Theme changed

                // Listen to any event that changes the layout (text changes, scrolling, etc)
                view.LayoutChanged += OnLayoutChanged;
            }
            else
            {
                Settings.Instance.OnShowCodeCoveragePropertyChanged -= Instance_OnShowCodeCoveragePropertyChanged;
                Settings.Instance.OnSettingsChanged -= Instance_OnSettingsChanged;
                Settings.Instance.RedrawNeeded -= Instance_OnRedrawNeeded;
                Settings.Instance.CleanNeeded -= Instance_OnClean;
                view.LayoutChanged -= OnLayoutChanged;
                VSColorTheme.ThemeChanged -= HandleThemeChange; // When VS Theme changed
            }
        }

        private void HandleThemeChange(ThemeChangedEventArgs e)
        {
            Microsoft.VisualStudio.Shell.ThreadHelper.ThrowIfNotOnUIThread();

            InitializeColors();
            Redraw();
        }

        /// <summary>
        /// Acts on the event of a redraw need
        /// </summary>
        private void Instance_OnRedrawNeeded()
        {
            ThreadHelper.JoinableTaskFactory.Run(async delegate ()
            {
                await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();
                Redraw();
            });
        }
        /// <summary>
        /// Acts on the event of a clean
        /// </summary>
        private void Instance_OnClean()
        {
            ThreadHelper.JoinableTaskFactory.Run(async delegate ()
            {
                await ThreadHelper.JoinableTaskFactory.SwitchToMainThreadAsync();

                currentProfile = null;
                activeReport = null;
                currentReportDate = DateTime.MinValue;
                layer.RemoveAllAdornments();

                Redraw();
            });
        }

        public void Close()
        {
            if (view != null)
            {
                SetupHandleEvents(false);
                view = null;
            }
        }

        public void Dispose()
        {
            Close();
        }

        /// <summary>
        /// Acts on changes for the settings OnShowCodeCoverage
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Instance_OnShowCodeCoveragePropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            Microsoft.VisualStudio.Shell.ThreadHelper.ThrowIfNotOnUIThread();

            this.outputWindow.WriteDebugLine("Instance_OnShowCodeCoveragePropertyChanged");
            Redraw();
        }

        /// <summary>
        /// Acts on change for the settings color(s)
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Instance_OnSettingsChanged(object sender, EventArgs e)
        {
            Microsoft.VisualStudio.Shell.ThreadHelper.ThrowIfNotOnUIThread();

            this.outputWindow.WriteDebugLine("Instance_OnSettingsChanged");
            InitializeColors();
            Redraw();
        }

        /// <summary>
        /// Initializes the brushes, might also be used to reinitialize them
        /// </summary>
        private void InitializeColors()
        {
            var backgroundColor = VSColorTheme.GetThemedColor(EnvironmentColors.SystemWindowTextColorKey);

            System.Windows.Media.Color[] colors;
            if ((backgroundColor.R + backgroundColor.G + backgroundColor.B) / 3 < 127)
            {
                colors = new[] { Settings.Instance.UncoveredBrushColor,
                                 Settings.Instance.UncoveredPenColor,
                                 Settings.Instance.CoveredBrushColor,
                                 Settings.Instance.CoveredPenColor,
                                 Settings.Instance.PartialCoveredBrushColor,
                                 Settings.Instance.PartialCoveredPenColor,
                };
            }
            else
            {
                colors = new[] { Settings.Instance.UncoveredDarkBrushColor,
                                 Settings.Instance.UncoveredDarkPenColor,
                                 Settings.Instance.CoveredDarkBrushColor,
                                 Settings.Instance.CoveredDarkPenColor,
                                 Settings.Instance.PartialCoveredDarkBrushColor,
                                 Settings.Instance.PartialCoveredDarkPenColor
                };
            }

            // Color for uncovered code:
            if (uncoveredBrush?.Color != colors[0])
            {
                uncoveredBrush = new SolidColorBrush(colors[0]);
                uncoveredBrush.Freeze();
            }

            if (uncoveredPenBrush?.Color != colors[1])
            {
                uncoveredPenBrush = new SolidColorBrush(colors[1]);
                uncoveredPenBrush.Freeze();
                uncoveredPen = new Pen(uncoveredPenBrush, 0.5);
                uncoveredPen.Freeze();
            }

            // Color for covered code:
            if (coveredBrush?.Color != colors[2])
            {
                coveredBrush = new SolidColorBrush(colors[2]);
                coveredBrush.Freeze();
            }

            if (coveredPenBrush?.Color != colors[3])
            {
                coveredPenBrush = new SolidColorBrush(colors[3]);
                coveredPenBrush.Freeze();
                coveredPen = new Pen(coveredPenBrush, 0.5);
                coveredPen.Freeze();
            }

            // Color for partial covered code:
            if (partialCoveredBrush?.Color != colors[4])
            {
                partialCoveredBrush = new SolidColorBrush(colors[4]);
                partialCoveredBrush.Freeze();
            }

            if (partialCoveredPenBrush?.Color != colors[2])
            {
                partialCoveredPenBrush = new SolidColorBrush(colors[5]);
                partialCoveredPenBrush.Freeze();
                partialCoveredPen = new Pen(partialCoveredPenBrush, 0.5);
                partialCoveredPen.Freeze();
            }

            var penMediaColor = VSColorTheme.GetThemedColor(EnvironmentColors.SystemWindowTextColorKey);
            var penColor = System.Windows.Media.Color.FromArgb(penMediaColor.A, penMediaColor.R, penMediaColor.G, penMediaColor.B);
            classicPenBrush = new SolidColorBrush(penColor);
            classicPenBrush.Freeze();
        }

        /// <summary>
        /// Gets the active filename
        /// </summary>
        /// <returns></returns>
        public string GetActiveFilename()
        {
            try
            {
                var res = this.textDocumentFactory.TryGetTextDocument(view.TextBuffer, out TextDocument);
                return res ? TextDocument.FilePath : null;
            }
            catch
            {
                return null;
            }
        }

        /// <summary>
        /// Does a full redraw of the adornment layer
        /// </summary>
        private void Redraw()
        {
            UpdateLayer(view.TextViewLines);
        }

        /// <summary>
        /// Initializes the current coverage data
        /// </summary>
        private bool InitCurrent()
        {
            string activeFilename = GetActiveFilename();
            if (activeFilename == null) return false;

            var dataProvider = ReportManagerSingleton.Instance(dte);
            if (dataProvider == null) return false;

            var coverageData = dataProvider.UpdateData();
            if (coverageData == null) return false;

            DateTime activeFileLastWrite = File.GetLastWriteTimeUtc(activeFilename);
            if (coverageData.FileDate < activeFileLastWrite) return false;

            if (currentReportDate == coverageData.FileDate)
            {
                return true;
            }

            currentReportDate = coverageData.FileDate;
            activeReport = coverageData.GetData(activeFilename);
            if (activeReport == null)
            {
                outputWindow.WriteDebugLine("No report found for this file: {0}", activeFilename);
                return false;
            }
            else
                outputWindow.WriteDebugLine("Report found for this file: {0}", activeFilename);

            return true;
        }

        /// <summary>
        /// On layout change add the adornment to any reformatted lines
        /// </summary>
        private void OnLayoutChanged(object sender, TextViewLayoutChangedEventArgs e)
        {
            UpdateLayer(e.NewOrReformattedLines);
        }

        private void UpdateLayer(IList<ITextViewLine> lines)
        {
            if (Settings.Instance.ShowCodeCoverage)
            {
                if (activeReport != null && lines.Count == 0)
                {
                    Redraw();
                }
                else
                {
                    if (InitCurrent())
                    {
                        foreach (ITextViewLine line in lines)
                        {
                            HighlightCoverage(line);
                        }
                    }
                    else
                    {
                        currentProfile = null;
                        activeReport = null;
                        currentReportDate = DateTime.MinValue;
                        layer.RemoveAllAdornments();
                    }
                }
            }
            else
            {
                layer.RemoveAllAdornments();
            }
        }

        private CoverageState[] UpdateCoverageData(List<KeyValuePair<int, int>> data)
        {
            List<Tuple<int, int, int>> newdata = new List<Tuple<int, int, int>>();
            foreach (var item in data)
            {
                int line = item.Key;

                // case 1:
                //   10: 0
                //   0xfeefee: 10
                //   --> 10: #=2, cov=1
                //
                // case 2:
                //   10: 0
                //   10: 10 
                //   --> 10: 10.
                //
                // case 3:
                //   10: 0
                //   12: 2
                //   --> 10: 0, 12: 2

                if (newdata.Count > 0 && newdata[newdata.Count - 1].Item1 == line)
                {
                    newdata.Add(new Tuple<int, int, int>(item.Key, 1, item.Value > 0 ? 1 : 0));
                }
                else if (line >= 0xf00000)
                {
                    var last = newdata[newdata.Count - 1];
                    newdata[newdata.Count - 1] = new Tuple<int, int, int>(last.Item1, last.Item2 + 1, last.Item3 + ((item.Value > 0) ? 1 : 0));
                }
                else
                {
                    newdata.Add(new Tuple<int, int, int>(item.Key, 1, item.Value > 0 ? 1 : 0));
                }
            }

            if (newdata.Count == 0)
            {
                newdata.Add(new Tuple<int, int, int>(0, 1, 1));
            }

            newdata.Sort();
            int max = newdata[newdata.Count - 1].Item1 + 1;

            // Initialize everything to 'covered'.
            CoverageState[] lines = new CoverageState[max];
            for (int i = 0; i < lines.Length; ++i)
            {
                lines[i] = CoverageState.Covered;
            }

            int lastline = 0;
            CoverageState lastState = CoverageState.Covered;
            foreach (var item in newdata)
            {
                for (int i = lastline; i < item.Item1; ++i)
                {
                    lines[i] = lastState;
                }

                lastline = item.Item1;
                lastState = (item.Item3 == 0) ? CoverageState.Uncovered : (item.Item3 == item.Item2) ? CoverageState.Covered : CoverageState.Partially;
            }

            for (int i = lastline; i < lines.Length; ++i)
            {
                lines[i] = lastState;
            }

            return lines;
        }

        private void HighlightCoverage(ITextViewLine line)
        {
            if (view == null || activeReport == null || line == null || view.TextSnapshot == null) { return; }

            IWpfTextViewLineCollection textViewLines = view.TextViewLines;

            if (textViewLines == null || line.Extent == null) { return; }

            int offsetPosition = VsVersion.Vs2022OrLater ? 0 : 1;
            int lineno = offsetPosition + view.TextSnapshot.GetLineNumberFromPosition(line.Extent.Start);

            CoverageState covered = lineno < activeReport.nbLines() ? activeReport.state((uint)lineno) : CoverageState.Irrelevant;

            if (covered != CoverageState.Irrelevant)
            {
                SnapshotSpan span = new SnapshotSpan(view.TextSnapshot, Span.FromBounds(line.Start, line.End));
                Geometry g = textViewLines.GetMarkerGeometry(span);

                if (g != null)
                {
                    var rectG = new RectangleGeometry(new Rect(g.Bounds.X, g.Bounds.Y, view.ViewportWidth, g.Bounds.Height));

                    GeometryDrawing drawing;
                    switch (covered)
                    {
                        case CoverageState.Covered:
                            drawing = new GeometryDrawing(coveredBrush, coveredPen, rectG);
                            break;
                        case CoverageState.Partially:
                            drawing = new GeometryDrawing(partialCoveredBrush, partialCoveredPen, rectG);
                            break;
                        default:
                            drawing = new GeometryDrawing(uncoveredBrush, uncoveredPen, rectG);
                            break;
                    }
                    drawing.Freeze();

                    DrawingImage drawingImage = new DrawingImage(drawing);
                    drawingImage.Freeze();

                    Image image = new Image
                    {
                        Source = drawingImage
                    };

                    //Align the image with the top of the bounds of the text geometry
                    Canvas.SetLeft(image, rectG.Bounds.Left);
                    Canvas.SetTop(image, rectG.Bounds.Top);

                    layer.AddAdornment(AdornmentPositioningBehavior.TextRelative, span, null, image, null);

                    // Show count if possible
                    if (covered != CoverageState.Uncovered && activeReport.hasCounting())
                    {
                        System.Text.StringBuilder sb = new System.Text.StringBuilder();
                        //sb.Append("Count=");
                        sb.Append(activeReport.count((uint)lineno));

                        double widthCount = 100.0;
                        double x = view.ViewportWidth - widthCount;
                        //double x = g.Bounds.X + g.Bounds.Width + 20;
                        //if (x < view.ViewportWidth / 2) { x = view.ViewportWidth / 2; }
                        var rectCountG = new RectangleGeometry(new Rect(x, g.Bounds.Y, widthCount, g.Bounds.Height));

                        Label lbl = new Label
                        {
                            FontSize = 8,
                            Foreground = classicPenBrush,
                            Background = Brushes.Transparent,
                            FontFamily = new FontFamily("Verdana"),
                            FontWeight = FontWeights.Bold,
                            Content = sb.ToString()
                        };

                        Canvas.SetLeft(lbl, rectCountG.Bounds.Left);
                        Canvas.SetTop(lbl, rectCountG.Bounds.Top);

                        layer.AddAdornment(AdornmentPositioningBehavior.TextRelative, span, null, lbl, null);
                    }
                }
            }

            if (currentProfile != null)
            {
                var profile = currentProfile.Get(lineno);
                if (profile != null && profile.Item1 != 0 || profile.Item2 != 0)
                {
                    System.Text.StringBuilder sb = new System.Text.StringBuilder();
                    sb.Append(profile.Item1);
                    sb.Append("%/");
                    sb.Append(profile.Item2);
                    sb.Append("%");

                    SnapshotSpan span = new SnapshotSpan(view.TextSnapshot, Span.FromBounds(line.Start, line.End));
                    Geometry g = textViewLines.GetMarkerGeometry(span);

                    if (g != null) // Yes, this apparently happens...
                    {
                        double x = g.Bounds.X + g.Bounds.Width + 20;
                        if (x < view.ViewportWidth / 2) { x = view.ViewportWidth / 2; }
                        g = new RectangleGeometry(new Rect(x, g.Bounds.Y, 30, g.Bounds.Height));

                        Label lbl = new Label
                        {
                            FontSize = 7,
                            Foreground = Brushes.Black,
                            Background = Brushes.Transparent,
                            FontFamily = new FontFamily("Verdana"),
                            Content = sb.ToString()
                        };

                        Canvas.SetLeft(lbl, g.Bounds.Left);
                        Canvas.SetTop(lbl, g.Bounds.Top);

                        layer.AddAdornment(AdornmentPositioningBehavior.TextRelative, span, null, lbl, null);
                    }
                }
            }
        }
    }
}
