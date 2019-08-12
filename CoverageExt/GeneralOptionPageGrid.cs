using Microsoft.VisualStudio.Shell;
using System;
using System.ComponentModel;
using System.Drawing;

namespace NubiloSoft.CoverageExt
{
    /// <summary>
    /// Dumbfounded why we need this, since it effectievely shouldn't do anything else but even 
    /// using [TypeConverter(typeof(ColorConverter))] doesn't work.
    /// </summary>
    class CustomColorConverter : ColorConverter { }

    [System.ComponentModel.DesignerCategory("")]
    public class GeneralOptionPageGrid : DialogPage
    {
        [Category("General")]
        [DisplayName("Show code coverage")]
        [Description("Should we show the code coverage or not")]
        public bool ShowCodeCoverage { get; set; } = true;

        [Category("Colors")]
        [DisplayName("Uncovered Brush")]
        [Description("Uncovered Brush")]
        [TypeConverter(typeof(CustomColorConverter))]
        public Color UncoveredBrush { get; set; } = Color.FromArgb(0xFF, 0xFF, 0xCF, 0xB8);

        [Category("Colors")]
        [DisplayName("Uncovered Pen")]
        [Description("Uncovered Pen")]
        [TypeConverter(typeof(CustomColorConverter))]
        public Color UncoveredPen { get; set; } = Color.FromArgb(0xD0, 0xFF, 0xCF, 0xB8);

        [Category("Colors")]
        [DisplayName("Covered Brush")]
        [Description("Covered Brush")]
        [TypeConverter(typeof(CustomColorConverter))]
        public Color CoveredBrush { get; set; } = Color.FromArgb(0xFF, 0xBD, 0xFC, 0xBF);

        [Category("Colors")]
        [DisplayName("Covered Pen")]
        [Description("Covered Pen")]
        [TypeConverter(typeof(CustomColorConverter))]
        public Color CoveredPen { get; set; } = Color.FromArgb(0xD0, 0xBD, 0xFC, 0xBF);

        public override void LoadSettingsFromStorage()
        {
            base.LoadSettingsFromStorage();
            UpdateSettings();
        }

        public override void SaveSettingsToStorage()
        {
            base.SaveSettingsToStorage();
            UpdateSettings();
        }

        public void UpdateSettings()
        {
            Func<Color, System.Windows.Media.Color> convert = (Color input) =>
            {
                return System.Windows.Media.Color.FromArgb(input.A, input.R, input.G, input.B);
            };

            Settings.Instance.ShowCodeCoverage = ShowCodeCoverage;
            Settings.Instance.UncoveredBrushColor = convert(UncoveredBrush);
            Settings.Instance.UncoveredPenColor = convert(UncoveredPen);
            Settings.Instance.CoveredBrushColor = convert(CoveredBrush);
            Settings.Instance.CoveredPenColor = convert(CoveredPen);
        }
    }
}
