using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Media;

namespace NubiloSoft.CoverageExt
{
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
        private bool useNativeCoverageSupport = true;
        public bool UseNativeCoverageSupport
        {
            get { return useNativeCoverageSupport; }
            set => SetField(ref useNativeCoverageSupport, value);
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

        #region color definitions
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

    }
}
