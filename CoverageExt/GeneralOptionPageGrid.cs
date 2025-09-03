using Microsoft.VisualStudio.Shell;
using System;
using System.ComponentModel;
using System.Drawing;
using System.Globalization;

namespace NubiloSoft.CoverageExt
{
    /// <summary>
    /// Dumbfounded why we need this, since it effectievely shouldn't do anything else but even 
    /// using [TypeConverter(typeof(ColorConverter))] doesn't work.
    /// </summary>
    class CustomColorConverter : ColorConverter {
        public override object ConvertFrom( ITypeDescriptorContext context, CultureInfo culture, object value )
        {
            return base.ConvertFrom(context, CultureInfo.InvariantCulture, value);
        }

        public override object ConvertTo( ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType )
        {
            return base.ConvertTo( context, CultureInfo.InvariantCulture, value, destinationType );
        }
    }

    [System.ComponentModel.DesignerCategory("")]
    public class GeneralOptionPageGrid : DialogPage
    {
        [Category("General")]
        [DisplayName("Show code coverage")]
        [Description("Should we show the code coverage or not")]
        [DefaultValue(true)]
        public bool ShowCodeCoverage { get; set; } = true;

        [Category("General")]
        [DisplayName("Build project before running")]
        [Description("Build project and dependencies before run code coverage")]
        [DefaultValue(false)]
        public bool CompileBeforeRunning { get; set; } = false;

        [Category("General")]
        [DisplayName("Coverage Format")]
        [Description("Coverage Format used to produce file (WARNING: need to use Native or NativeV2 to be show in visual studio")]
        [DefaultValue(CoverageFormat.NativeV2)]
        [TypeConverter(typeof(CoverageFormat))]
        public CoverageFormat Format { get; set; } = CoverageFormat.NativeV2;

        [Category("General")]
        [DisplayName("Use OpenCppCoverage")]
        [Description("Replace native coverage system by OpenCppCoverage. Must be into C:\\Program files\\")]
        [DefaultValue(false)]
        public bool OpenCppCoverage { get; set; } = false;

        [Category("Bright Colors")]
        [DisplayName("Uncovered Brush")]
        [Description("Uncovered Brush")]
        [TypeConverter(typeof(CustomColorConverter))]
        public Color UncoveredBrush { get; set; } = Color.FromArgb(0xFF, 0xFF, 0xCF, 0xB8);

        [Category("Bright Colors")]
        [DisplayName("Uncovered Pen")]
        [Description("Uncovered Pen")]
        [TypeConverter(typeof(CustomColorConverter))]
        public Color UncoveredPen { get; set; } = Color.FromArgb(0xD0, 0xFF, 0xCF, 0xB8);

        [Category("Bright Colors")]
        [DisplayName("Covered Brush")]
        [Description("Covered Brush")]
        [TypeConverter(typeof(CustomColorConverter))]
        public Color CoveredBrush { get; set; } = Color.FromArgb(0xFF, 0xBD, 0xFC, 0xBF);

        [Category("Bright Colors")]
        [DisplayName("Covered Pen")]
        [Description("Covered Pen")]
        [TypeConverter(typeof(CustomColorConverter))]
        public Color CoveredPen { get; set; } = Color.FromArgb(0xD0, 0xBD, 0xFC, 0xBF);

        [Category("Dark Colors")]
        [DisplayName("Uncovered Brush")]
        [Description("Uncovered Brush")]
        [TypeConverter(typeof(CustomColorConverter))]
        public Color UncoveredDarkBrush { get; set; } = Color.FromArgb(0xFF, 0x30, 0x05, 0x05);

        [Category("Dark Colors")]
        [DisplayName("Uncovered Pen")]
        [Description("Uncovered Pen")]
        [TypeConverter(typeof(CustomColorConverter))]
        public Color UncoveredDarkPen { get; set; } = Color.FromArgb(0x00, 0x30, 0x05, 0x05);

        [Category("Dark Colors")]
        [DisplayName("Covered Brush")]
        [Description("Covered Brush")]
        [TypeConverter(typeof(CustomColorConverter))]
        public Color CoveredDarkBrush { get; set; } = Color.FromArgb(0xFF, 0x05, 0x30, 0x05);

        [Category("Dark Colors")]
        [DisplayName("Covered Pen")]
        [Description("Covered Pen")]
        [TypeConverter(typeof(CustomColorConverter))]
        public Color CoveredDarkPen { get; set; } = Color.FromArgb(0x00, 0x05, 0x30, 0x05);

        protected override void OnApply( PageApplyEventArgs e )
        {
            base.OnApply(e);
            UpdateSettings();
        }

        public void UpdateSettings()
        {
            Func<Color, System.Windows.Media.Color> convert = (Color input) =>
            {
                return System.Windows.Media.Color.FromArgb(input.A, input.R, input.G, input.B);
            };

            Settings.Instance.ShowCodeCoverage = ShowCodeCoverage;
            Settings.Instance.CompileBeforeRunning = CompileBeforeRunning;
            Settings.Instance.UncoveredBrushColor   = convert(UncoveredBrush);
            Settings.Instance.UncoveredPenColor     = convert(UncoveredPen);
            Settings.Instance.CoveredBrushColor     = convert(CoveredBrush);
            Settings.Instance.CoveredPenColor       = convert(CoveredPen);

            Settings.Instance.UncoveredDarkBrushColor   = convert(UncoveredDarkBrush);
            Settings.Instance.UncoveredDarkPenColor     = convert(UncoveredDarkPen);
            Settings.Instance.CoveredDarkBrushColor     = convert(CoveredDarkBrush);
            Settings.Instance.CoveredDarkPenColor       = convert(CoveredDarkPen);
            Settings.Instance.TriggerSettingsChanged();
        }
    }
}
