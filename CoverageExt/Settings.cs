using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Media;

namespace NubiloSoft.CoverageExt
{
    public enum CoverageFormat
    {
        Native,
        NativeV2,
        //Cobertura,
        //Clover
    }

    public class Settings
    {
        private static readonly Settings instance = new Settings();
        private Settings() { }

        public static Settings Instance { get => instance; }

        private bool propertyChanged = true;

        protected bool SetField<T>(ref T field, T value)
        {
            if (EqualityComparer<T>.Default.Equals(field, value)) return false;
            field = value;
            propertyChanged = true;
            return true;
        }

        #region general Events
        public event Action RedrawNeeded;
        public void TriggerRedraw()
        {
            if (RedrawNeeded != null)
            {
                RedrawNeeded();
            }
        }


        public event Action CleanNeeded;
        public void TriggerCleanNeeded()
        {
            if (CleanNeeded != null)
            {
                CleanNeeded();
            }
        }

        public event EventHandler OnSettingsChanged;
        public void TriggerSettingsChanged()
        {
            if (propertyChanged) {
                OnSettingsChanged?.Invoke(this, EventArgs.Empty);
                propertyChanged = false;
            }
        }

        public event PropertyChangedEventHandler OnShowCodeCoveragePropertyChanged;
        public void ToggleShowCodeCoverage()
        {
            this.ShowCodeCoverage = !this.ShowCodeCoverage;
            OnShowCodeCoveragePropertyChanged?.Invoke(this, new PropertyChangedEventArgs(null));
            propertyChanged = false;
        }
        #endregion

        #region general properties
        private CoverageFormat format = CoverageFormat.NativeV2;
        public CoverageFormat Format
        {
            get { return format; }
            set => SetField(ref format, value);
        }

        private bool useOpenCppCoverageRunner = false;
        public bool UseOpenCppCoverageRunner
        {
            get => this.useOpenCppCoverageRunner;
            set => SetField(ref useOpenCppCoverageRunner, value);
        }

        private bool showCodeCoverage = false;
        public bool ShowCodeCoverage
        {
            get => this.showCodeCoverage;
            set => SetField(ref showCodeCoverage, value);
        }

        private bool compileBeforeRunning = false;
        public bool CompileBeforeRunning
        {
            get => this.compileBeforeRunning;
            set => SetField(ref compileBeforeRunning, value);
        }
        #endregion

        #region Bright color definitions
        private Color uncoveredBrushColor;
        public Color UncoveredBrushColor
        {
            get => this.uncoveredBrushColor;
            set => SetField(ref uncoveredBrushColor, value);
        }

        private Color uncoveredPenColor = Color.FromArgb(0xD0, 0xFF, 0xCF, 0xB8);
        public Color UncoveredPenColor
        {
            get => this.uncoveredPenColor;
            set => SetField(ref uncoveredPenColor, value);
        }

        private Color coveredBrushColor;
        public Color CoveredBrushColor
        {
            get => this.coveredBrushColor;
            set => SetField(ref coveredBrushColor, value);
        }

        private Color coveredPenColor = Color.FromArgb(0xD0, 0xBD, 0xFC, 0xBF);
        public Color CoveredPenColor
        {
            get => this.coveredPenColor;
            set => SetField(ref coveredPenColor, value);
        }
        #endregion

        #region Dark color definitions
        private Color uncoveredDarkBrushColor;
        public Color UncoveredDarkBrushColor
        {
            get => this.uncoveredDarkBrushColor;
            set => SetField(ref uncoveredDarkBrushColor, value);
        }

        private Color uncoveredDarkPenColor = Color.FromArgb(0xFF, 0x30, 0x00, 0x00);
        public Color UncoveredDarkPenColor
        {
            get => this.uncoveredDarkPenColor;
            set => SetField(ref uncoveredDarkPenColor, value);
        }

        private Color coveredDarkBrushColor;
        public Color CoveredDarkBrushColor
        {
            get => this.coveredDarkBrushColor;
            set => SetField(ref coveredDarkBrushColor, value);
        }

        private Color coveredDarkPenColor = Color.FromArgb(0xFF, 0x00, 0x30, 0x00);
        public Color CoveredDarkPenColor
        {
            get => this.coveredDarkPenColor;
            set => SetField(ref coveredDarkPenColor, value);
        }
        #endregion

    }
}
