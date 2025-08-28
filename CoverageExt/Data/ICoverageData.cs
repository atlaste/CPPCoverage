using System;
using System.Collections.Generic;

namespace NubiloSoft.CoverageExt.Data
{
    public enum CoverageState : Int16
    {
        Irrelevant,
        Covered,
        Partially,
        Uncovered
    }

    public class FileCoverageStats
    {
        public UInt32 lineInsideFile  = 0;   // All lines
        public UInt32 lineOfCodeFile  = 0;   // Lines where status coverage is possible
        public UInt32 lineCoveredFile = 0;   // Lines covered or partial
    }

    public interface IFileCoverageData
    {
        CoverageState state(UInt32 idLine);
        UInt32 count(UInt32 idLine);
        UInt32 nbLines();
        ProfileVector profile();
    }

    public interface ICoverageData
    {
        //Tuple<BitVector, ProfileVector> GetData(string filename);
        IFileCoverageData GetData(string filename);

        DateTime FileDate { get; }
        IEnumerable<Tuple<string, FileCoverageStats>> Overview();

        void Parsing(string filename, string solutionDir);
    }
}
