using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows.Media;

namespace NubiloSoft.CoverageExt
{
    public class Settings
    {
        private static readonly Settings instance = new Settings();
        private Settings() { }

        public static Settings Instance { get => instance; }

        protected bool SetField<T>(ref T field, T value, PropertyChangedEventHandler propertyChanged = null, [CallerMemberName] string propertyName = null)
        {
            if (EqualityComparer<T>.Default.Equals(field, value)) return false;
            field = value;
            propertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
            return true;
        }

        #region general Events
        public event Action RedrawNeeded;
        public void TriggerRedraw()
        {
            RedrawNeeded();
        }
        #endregion

        #region general properties
        private bool useNativeCoverageSupport = true;
        public bool UseNativeCoverageSupport
        {
            get { return useNativeCoverageSupport; }
            set => SetField(ref useNativeCoverageSupport, value);
        }

        public event PropertyChangedEventHandler OnShowCodeCoveragePropertyChanged;
        private bool showCodeCoverage = true;
        public bool ShowCodeCoverage
        {
            get => this.showCodeCoverage;
            set => SetField(ref showCodeCoverage, value, OnShowCodeCoveragePropertyChanged);
        }
        #endregion

        #region color definitions
        public event PropertyChangedEventHandler OnColorPropertyChanged;

        private Color uncoveredBrushColor;
        public Color UncoveredBrushColor
        {
            get => this.uncoveredBrushColor;
            set => SetField(ref uncoveredBrushColor, value, OnColorPropertyChanged);
        }

        private Color uncoveredPenColor = Color.FromArgb(0xD0, 0xFF, 0xCF, 0xB8);
        public Color UncoveredPenColor
        {
            get => this.uncoveredPenColor;
            set => SetField(ref uncoveredPenColor, value, OnColorPropertyChanged);
        }

        private Color coveredBrushColor;
        public Color CoveredBrushColor
        {
            get => this.coveredBrushColor;
            set => SetField(ref coveredBrushColor, value, OnColorPropertyChanged);
        }

        private Color coveredPenColor = Color.FromArgb(0xD0, 0xBD, 0xFC, 0xBF);
        public Color CoveredPenColor
        {
            get => this.coveredPenColor;
            set => SetField(ref coveredPenColor, value, OnColorPropertyChanged);
        }
        #endregion

    }
}
