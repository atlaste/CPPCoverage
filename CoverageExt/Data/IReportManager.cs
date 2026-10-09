namespace NubiloSoft.CoverageExt.Data
{
    public interface IReportManager
    {
        ICoverageData UpdateData();
        void ResetData();
    }
}
