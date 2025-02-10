using NubiloSoft.CoverageExt.Data;
using System.IO;

namespace NubiloSoft.CoverageExt.Cobertura
{
    /// <summary>
    /// This class post-processes the coverage results. We basically scan each file for "#pragma EnableCodeCoverage" 
    /// and "#pragma DisableCodeCoverage" and if we encounter this, we'll update the bit vector.
    /// </summary>
    public class HandlePragmas
    {
        private static readonly string PRAGMA_LINE = "#pragma";

        private enum LineType
        {
            CODE,
            ENABLE_COVERAGE,
            DISABLE_COVERAGE
        }

        private static LineType GetLineType( string line )
        {
            string lineTrim = line.TrimStart();
            if (lineTrim.StartsWith(PRAGMA_LINE))
            {
                int idx = PRAGMA_LINE.Length;
                string t = lineTrim.Substring(idx).TrimStart();
                if (t == "EnableCodeCoverage")
                {
                    return LineType.ENABLE_COVERAGE;
                }
                else if (t == "DisableCodeCoverage")
                {
                    return LineType.DISABLE_COVERAGE;
                }
            }
            return LineType.CODE;
        }

        public static void Update(string filename, BitVector data)
        {
            bool enabled = true;

            try
            {
                string[] lines = File.ReadAllLines(filename);
                for (int i = 0; i < lines.Length; ++i)
                {
                    var flag = GetLineType(lines[i]);
                    if (flag == LineType.ENABLE_COVERAGE)
                    {
                        data.Remove(i);
                        enabled = true;
                    }
                    else if (flag == LineType.DISABLE_COVERAGE)
                    {
                        enabled = false;
                    }

                    // Update data accordingly:
                    if (!enabled)
                    {
                        data.Remove(i);
                    }
                }

                if (!enabled)
                {
                    var count = data.Count;
                    for (int i = lines.Length; i < count; ++i)
                    {
                        data.Remove(i);
                    }
                }
            }
            catch { } // we don't want this to crash our program.
        }
    }
}
