using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using EnvDTE;
using System.Collections.ObjectModel;

namespace NubiloSoft.CoverageExt.Report
{
    /// <summary>
    /// Interaction logic for MyControl.xaml
    /// </summary>
    public partial class CoverageReport : UserControl
    {
        public CoverageReport(Func<EnvDTE.DTE> dte)
        {
            InitializeComponent();

            this.dte = dte;
        }

        private Data.IReportManager Manager
        {
            get
            {
                var d = dte();
                return d == null ? null : Data.ReportManagerSingleton.Instance(d);
            }
        }

        private Func<EnvDTE.DTE> dte;

        private static object lockObject = new object();

        private void UserControl_Loaded(object sender, RoutedEventArgs e)
        {
            // First update the data.
            Update();

            // Link data to UI.
            CollectionViewSource itemCollectionViewSource = (CollectionViewSource)(FindResource("ItemCollectionViewSource"));
            itemCollectionViewSource.Source = new ObservableCollection<FileCoverage>();
            itemCollectionViewSource.Source = this.loaded;

            BindingOperations.EnableCollectionSynchronization(this.loaded, lockObject);
        }

        private List<FileCoverage> loaded = new List<FileCoverage>();

        public List<FileCoverage> LoadedData { get { return loaded; } }

        public class FileCoverage
        {
            public string Name { get; set; }
            public string Filename { get; set; }
            public int Total { get; set; }
            public int Count { get; set; }

            public int Percentage
            {
                get
                {
                    return Count * 100 / Total;
                }
            }

            public Brush Color
            {
                get
                {
                    int frac = 55 + (Count * 200) / Total;
                    if (frac < 0) { frac = 0; } else if (frac > 255) { frac = 255; }
                    return (SolidColorBrush)(new BrushConverter().ConvertFrom("#ff" + (frac.ToString("x2")) + (frac.ToString("x2"))));
                }
            }
        }

        private void Load()
        {
            try
            {
                LoadImpl();
            }
            catch { }
        }

        private void LoadImpl()
        {
            var manager = Manager;
            if (manager == null)
            {
                return;
            }

            var report = manager.UpdateData();
            if (report == null)
            {
                return;
            }

            if (report == null)
            {
                return;
            }

            string lcs = null;
            Dictionary<string, FileCoverage> dict = new Dictionary<string, FileCoverage>();
            foreach (var item in report.Overview().OrderBy((a) => a.Item1))
            {
                // Strip a common prefix from all strings.
                if (!string.IsNullOrEmpty(item.Item1))
                {
                    if (lcs == null)
                    {
                        lcs = System.IO.Path.GetDirectoryName(item.Item1);
                        if (!lcs.EndsWith("\\")) { lcs += "\\"; }
                    }
                    else
                    {
                        string rhs = System.IO.Path.GetDirectoryName(item.Item1);

                        if (!rhs.EndsWith("\\")) { rhs += "\\"; }
                        if (!rhs.StartsWith(lcs))
                        {
                            int common = 0;
                            int limit = Math.Min(rhs.Length, lcs.Length);
                            for (int i = 0; i < limit && rhs[i] == lcs[i]; ++i)
                            {
                                if (lcs[i] == '\\' || lcs[i] == '/')
                                {
                                    common = i;
                                }
                            }
                            lcs = rhs.Substring(0, common + 1);
                        }
                    }

                    dict.Add(item.Item1,
                        new FileCoverage()
                        {
                            Filename = item.Item1,
                            Name = null,
                            Count = item.Item2,
                            Total = item.Item2 + item.Item3
                        });
                }
            }

            foreach (var item in dict)
            {
                item.Value.Name = item.Value.Filename.Substring(lcs.Length);
            }

            // Calculate grand total:
            int count = 0;
            int total = 0;
            foreach (var item in dict)
            {
                if (item.Value.Filename.IndexOf(".test", StringComparison.InvariantCultureIgnoreCase) < 0)
                {
                    count += item.Value.Count;
                    total += item.Value.Total;
                }
            }

            lock (lockObject) // is this okay?
            {
                this.loaded.Clear();

                if (total != 0)
                {
                    this.loaded.Add(new FileCoverage() { Count = count, Total = total, Name = "Grand total", Filename = null });
                }

                this.loaded.Clear();
                foreach (var item in dict.
                    OrderBy((a)=>a.Key).
                    Where((a) => a.Value.Filename.IndexOf(".test", StringComparison.InvariantCultureIgnoreCase) < 0 && a.Value.Total > 0).
                    Select((a) => a.Value))
                {
                    this.loaded.Add(item);
                }
            }
        }

        private void MyToolWindow_GotFocus(object sender, RoutedEventArgs e)
        {
            var d = dte();
            if (d != null)
            {
                Load();
            }
        }

        private void DataGrid_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            if (sender != null)
            {
                DataGrid grid = sender as DataGrid;
                var fc = grid.SelectedItem as FileCoverage;
                if (fc != null && fc.Filename != null)
                {
                    string filename = fc.Filename;

                    // Open.
                    var d = dte();
                    if (d != null && d.ItemOperations != null)
                    {
                        d.ItemOperations.OpenFile(filename, Constants.vsViewKindTextView);
                    }
                }
            }
        }

        public void Update()
        {
            Load();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            Update();
        }

        private void Grid_GotFocus(object sender, RoutedEventArgs e)
        {
            Update();
        }

        private void MyToolWindow_LayoutUpdated(object sender, EventArgs e)
        {
        }

        private void MyToolWindow_RequestBringIntoView(object sender, RequestBringIntoViewEventArgs e)
        {
            Update();
        }
    }
}