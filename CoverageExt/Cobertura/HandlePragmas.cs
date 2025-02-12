using NubiloSoft.CoverageExt.Data;
using System;
using System.IO;

namespace NubiloSoft.CoverageExt.Cobertura
{
    /// <summary>
    /// This class post-processes the coverage results. We basically scan each file for "#pragma EnableCodeCoverage" 
    /// and "#pragma DisableCodeCoverage" and if we encounter this, we'll update the bit vector.
    /// WARNING: If you use high level of warning (level 4), you need to disable the following code 4068 or the "#pragma warning (disable:4068)".
    ///
    /// To avoid it, this system works using following SINGLE-LINE comment too: "// EnableCodeCoverage" and "// DisableCodeCoverage".
    ///
    /// Also available is the placement of flags in MULTI-LINE comment.
    /// The condition is to put the flag on the same line as the start of the MULTI_LINE comment:
    /// "/* EnableCodeCoverage" and "/* DisableCodeCoverage".
    /// </summary>
    public class HandlePragmas
    {
        private static string DISABLE_COVERAGE = "DisableCodeCoverage";
        private static string ENABLE_COVERAGE = "EnableCodeCoverage";
        private static string[] prefixCoverage = new string[] { "#pragma", "//", "/*" };
        private static string FORWARD_SLASH_ASTERISK_LINE_END = "*/";

        private enum LineType
        {
            CODE,
            ENABLE_COVERAGE,
            DISABLE_COVERAGE
        }

        private static bool IsCoverageFlag( ReadOnlySpan<char> lineSpan, string coverageFlag, bool multilineComment )
        {
            if (lineSpan.Length < coverageFlag.Length)
            {
                return false;
            }
            if (lineSpan.Length == coverageFlag.Length)
            {
                return lineSpan.StartsWith(coverageFlag.AsSpan());
            }

            if (!multilineComment)
            {
                return false;
            }

            if (!lineSpan.StartsWith(coverageFlag.AsSpan()))
            {
                return false;
            }

            int restSize = lineSpan.Length - coverageFlag.Length;
            if (restSize < FORWARD_SLASH_ASTERISK_LINE_END.Length)
            {
                return false;
            }

            return lineSpan.Slice(coverageFlag.Length, restSize).StartsWith(FORWARD_SLASH_ASTERISK_LINE_END.AsSpan());
        }

        private static LineType GetLineType( string line )
        {
            foreach (var prefix in prefixCoverage) {
                int idx = line.IndexOf(prefix);
                if (idx >= 0)
                {
                    idx += prefix.Length;
                    while (idx < line.Length && char.IsWhiteSpace(line[idx])) idx++;
                    int jdx = idx;
                    while (jdx < line.Length && !char.IsWhiteSpace(line[jdx])) jdx++;

                    ReadOnlySpan<char> lineSpan = line.AsSpan().Slice(idx, jdx - idx);
                    bool multiLineComment = prefix == "/*";

                    if (IsCoverageFlag(lineSpan, ENABLE_COVERAGE, multiLineComment))
                    {
                        return LineType.ENABLE_COVERAGE;
                    }
                    if (IsCoverageFlag(lineSpan, DISABLE_COVERAGE, multiLineComment))
                    {
                        return LineType.DISABLE_COVERAGE;
                    }
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
                    string line = lines[i];
                    var flag = GetLineType(line);
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
