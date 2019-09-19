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
    public enum CoverageState : int
    {
        Irrelevant,
        Covered,
        Partially,
        Uncovered
    }

    public class CodeCoverage : IDisposable
    {
        internal IAdornmentLayer layer;
        internal IWpfTextView view;

        internal Brush uncoveredBrush;
        internal Pen uncoveredPen;
        internal Brush coveredBrush;
        internal Pen coveredPen;

        private EnvDTE.DTE dte;

        private CoverageState[] currentCoverage;
        private ProfileVector currentProfile;

        public CodeCoverage(IWpfTextView view, EnvDTE.DTE dte)
        {
            this.dte = dte;
            this.view = view;
            this.layer = view.GetAdornmentLayer("CodeCoverage");
            this.layer.Opacity = 0.4;

            SetupHandleEvents(true);

            // make sure the burshes are atleast initialized once
            InitializeColors();
        }

        private void SetupHandleEvents(bool setup)
        {
            if (setup)
            {
                // listen to events that change the setting properties
                Settings.Instance.OnShowCodeCoveragePropertyChanged += Instance_OnShowCodeCoveragePropertyChanged;
                Settings.Instance.OnColorPropertyChanged += Instance_OnColorPropertyChanged;
                Settings.Instance.RedrawNeeded += Instance_OnRedrawNeeded;

                // Listen to any event that changes the layout (text changes, scrolling, etc)
                view.LayoutChanged += OnLayoutChanged;
            }
            else
            {
                Settings.Instance.OnShowCodeCoveragePropertyChanged -= Instance_OnShowCodeCoveragePropertyChanged;
                Settings.Instance.OnColorPropertyChanged -= Instance_OnColorPropertyChanged;
                Settings.Instance.RedrawNeeded -= Instance_OnRedrawNeeded;
                view.LayoutChanged -= OnLayoutChanged;
            }
        }

        /// <summary>
        /// Acts on the event of a redraw need
        /// </summary>
        private void Instance_OnRedrawNeeded()
        {
            Redraw();
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
            Redraw();
        }

        /// <summary>
        /// Acts on change for the settings color(s)
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Instance_OnColorPropertyChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            InitializeColors();
            Redraw();
        }

        /// <summary>
        /// Initializes the brushes, might also be used to reinitialize them
        /// </summary>
        private void InitializeColors()
        {
            // Color for uncovered code:
            uncoveredBrush = new SolidColorBrush(Settings.Instance.UncoveredBrushColor);
            uncoveredBrush.Freeze();

            Brush penBrush = new SolidColorBrush(Settings.Instance.UncoveredPenColor);
            penBrush.Freeze();
            uncoveredPen = new Pen(penBrush, 0.5);
            uncoveredPen.Freeze();

            // Color for covered code:
            coveredBrush = new SolidColorBrush(Settings.Instance.CoveredBrushColor);
            coveredBrush.Freeze();

            penBrush = new SolidColorBrush(Settings.Instance.CoveredPenColor);
            penBrush.Freeze();
            coveredPen = new Pen(penBrush, 0.5);
            coveredPen.Freeze();
        }

        /// <summary>
        /// Gets the active filename
        /// </summary>
        /// <returns></returns>
        public string GetActiveFilename()
        {
            try
            {
                return dte.ActiveDocument.FullName;
            }
            catch
            {
                return null;
            }
        }

        /// <summary>
        /// Does a full redraw of the addornment layer
        /// </summary>
        private void Redraw()
        {
            if (Settings.Instance.ShowCodeCoverage)
            {
                InitCurrent();

                foreach (ITextViewLine line in view.TextViewLines)
                {
                    HighlightCoverage(currentCoverage, currentProfile, line);
                }
            }
            else
            {
                layer.RemoveAllAdornments();
            }
        }

        /// <summary>
        /// Initializes the current coverage data
        /// </summary>
        private void InitCurrent()
        {
            CoverageState[] currentFile = new CoverageState[0];
            ProfileVector currentProf = new Data.ProfileVector(0);

            if (Settings.Instance.ShowCodeCoverage)
            {
                string activeFilename = GetActiveFilename();
                if (activeFilename != null)
                {
                    Tuple<BitVector, ProfileVector> activeReport = null;
                    DateTime activeFileLastWrite = File.GetLastWriteTimeUtc(activeFilename);

                    var dataProvider = Data.ReportManagerSingleton.Instance(dte);
                    if (dataProvider != null)
                    {
                        var coverageData = dataProvider.UpdateData();
                        if (coverageData != null && activeFilename != null)
                        {
                            if (coverageData.FileDate >= activeFileLastWrite)
                            {
                                activeReport = coverageData.GetData(activeFilename);
                            }
                        }
                    }

                    if (activeReport != null)
                    {
                        currentProf = activeReport.Item2;
                        currentFile = new CoverageState[activeReport.Item1.Count];

                        foreach (var item in activeReport.Item1.Enumerate())
                        {
                            if (item.Value)
                            {
                                currentFile[item.Key] = CoverageState.Covered;
                            }
                            else
                            {
                                currentFile[item.Key] = CoverageState.Uncovered;
                            }
                        }
                    }
                }
            }

            this.currentCoverage = currentFile;
            this.currentProfile = currentProf;
        }

        /// <summary>
        /// On layout change add the adornment to any reformatted lines
        /// </summary>
        private void OnLayoutChanged(object sender, TextViewLayoutChangedEventArgs e)
        {
            InitCurrent();

            foreach (ITextViewLine line in e.NewOrReformattedLines)
            {
                HighlightCoverage(currentCoverage, currentProfile, line);
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

        private void HighlightCoverage(CoverageState[] coverdata, ProfileVector profiledata, ITextViewLine line)
        {
            if (view == null || profiledata == null || line == null || view.TextSnapshot == null) { return; }

            IWpfTextViewLineCollection textViewLines = view.TextViewLines;

            if (textViewLines == null || line.Extent == null) { return; }

            int lineno = 1 + view.TextSnapshot.GetLineNumberFromPosition(line.Extent.Start);

            CoverageState covered = lineno < coverdata.Length ? coverdata[lineno] : CoverageState.Irrelevant;

            if (covered != CoverageState.Irrelevant)
            {
                SnapshotSpan span = new SnapshotSpan(view.TextSnapshot, Span.FromBounds(line.Start, line.End));
                Geometry g = textViewLines.GetMarkerGeometry(span);

                if (g != null)
                {
                    g = new RectangleGeometry(new Rect(g.Bounds.X, g.Bounds.Y, view.ViewportWidth, g.Bounds.Height));

                    GeometryDrawing drawing = (covered == CoverageState.Covered) ?
                        new GeometryDrawing(coveredBrush, coveredPen, g) :
                        new GeometryDrawing(uncoveredBrush, uncoveredPen, g);

                    drawing.Freeze();

                    DrawingImage drawingImage = new DrawingImage(drawing);
                    drawingImage.Freeze();

                    Image image = new Image();
                    image.Source = drawingImage;

                    //Align the image with the top of the bounds of the text geometry
                    Canvas.SetLeft(image, g.Bounds.Left);
                    Canvas.SetTop(image, g.Bounds.Top);

                    layer.AddAdornment(AdornmentPositioningBehavior.TextRelative, span, null, image, null);
                }
            }

            var profile = profiledata.Get(lineno);
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

                    Label lbl = new Label();
                    lbl.FontSize = 7;
                    lbl.Foreground = Brushes.Black;
                    lbl.Background = Brushes.Transparent;
                    lbl.FontFamily = new FontFamily("Verdana");
                    lbl.Content = sb.ToString();

                    Canvas.SetLeft(lbl, g.Bounds.Left);
                    Canvas.SetTop(lbl, g.Bounds.Top);

                    layer.AddAdornment(AdornmentPositioningBehavior.TextRelative, span, null, lbl, null);
                }
            }
        }


    }
}
