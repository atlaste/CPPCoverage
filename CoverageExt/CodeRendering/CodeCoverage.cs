using System;
using System.Collections.Generic;
using System.IO;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Media;
using Microsoft.VisualStudio.Text;
using Microsoft.VisualStudio.Text.Editor;
using Microsoft.VisualStudio.Text.Formatting;
using NubiloSoft.CoverageExt.Data;

namespace NubiloSoft.CoverageExt.CodeRendering
{
    public enum CoverageState : int
    {
        Irrelevant,
        Covered,
        Partially,
        Uncovered
    }

    public class CodeCoverage
    {
        internal IAdornmentLayer layer;
        internal IWpfTextView view;

        internal Brush uncoveredBrush;
        internal Pen uncoveredPen;
        internal Brush coveredBrush;
        internal Pen coveredPen;

        private EnvDTE.DTE dte;

        private CoverageState[] currentCoverage;

        public CodeCoverage(IWpfTextView view, EnvDTE.DTE dte)
        {
            this.dte = dte;
            this.view = view;
            this.layer = view.GetAdornmentLayer("CodeCoverage");

            //Listen to any event that changes the layout (text changes, scrolling, etc)
            view.LayoutChanged += OnLayoutChanged;

            Brush brush = new SolidColorBrush(Color.FromArgb(0xF0, 0xF2, 0xE4, 0xDF));
            brush.Freeze();
            Brush penBrush = new SolidColorBrush(Color.FromArgb(0xB0, 0xF2, 0xE4, 0xDF));
            penBrush.Freeze();
            Pen pen = new Pen(penBrush, 0.5);
            pen.Freeze();

            uncoveredBrush = brush;
            uncoveredPen = pen;

            // Partially covered:
            brush = new SolidColorBrush(Color.FromArgb(0xF0, 0xE2, 0xF2, 0xDF));
            brush.Freeze();
            penBrush = new SolidColorBrush(Color.FromArgb(0xB0, 0xDA, 0xF2, 0xD5));
            penBrush.Freeze();
            pen = new Pen(penBrush, 0.5);
            pen.Freeze();

            coveredBrush = brush;
            coveredPen = pen;
        }

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
        /// On layout change add the adornment to any reformatted lines
        /// </summary>
        private void OnLayoutChanged(object sender, TextViewLayoutChangedEventArgs e)
        {
            CoverageState[] currentFile = new CoverageState[0];

            string activeFilename = GetActiveFilename();
            if (activeFilename != null)
            {
                BitVector activeReport = null;
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
                    currentFile = new CoverageState[activeReport.Count];

                    foreach (var item in activeReport.Enumerate())
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

            this.currentCoverage = currentFile;

            foreach (ITextViewLine line in e.NewOrReformattedLines)
            {
                HighlightCoverage(currentCoverage, line);
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

        private void HighlightCoverage(CoverageState[] coverdata, ITextViewLine line)
        {
            IWpfTextViewLineCollection textViewLines = view.TextViewLines;

            int lineno = 1 + view.TextSnapshot.GetLineNumberFromPosition(line.Extent.Start);

            CoverageState covered = lineno < coverdata.Length ? coverdata[lineno] : CoverageState.Irrelevant;

            if (covered != CoverageState.Irrelevant)
            {
                SnapshotSpan span = new SnapshotSpan(view.TextSnapshot, Span.FromBounds(line.Start, line.End));
                Geometry g = textViewLines.GetMarkerGeometry(span);
                if (g != null)
                {
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
        }
    }
}
