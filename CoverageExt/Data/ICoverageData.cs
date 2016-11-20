using System;
using System.Collections.Generic;

namespace NubiloSoft.CoverageExt.Data
{
    public interface ICoverageData
    {
        Tuple<BitVector, ProfileVector> GetData(string filename);
        DateTime FileDate { get; }
        IEnumerable<Tuple<string, int, int>> Overview();
    }
}
