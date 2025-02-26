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
    /// </summary>
    public class HandlePragmas
    {
        private static readonly string DISABLE_COVERAGE = "DisableCodeCoverage";
        private static readonly string ENABLE_COVERAGE = "EnableCodeCoverage";
        private static readonly string PRAGMA_LINE = "#pragma";
        private static readonly string DOUBLE_FORWARD_SLASH_LINE = "//";

        private enum LineType
        {
            CODE,
            ENABLE_COVERAGE,
            DISABLE_COVERAGE
        }

        private static bool IsCoverageFlag( ReadOnlySpan<char> lineSpan, string coverageFlag )
        {
            if (lineSpan.Length != coverageFlag.Length)
            {
                return false;
            }

            return lineSpan.StartsWith(coverageFlag.AsSpan());
        }

        private static int FindNotWhitespaceIndex( ReadOnlySpan<char> lineSpan, int offset )
        {
            int idx = offset;
            while (idx < lineSpan.Length && char.IsWhiteSpace(lineSpan[idx])) idx++;
            return idx;
        }

        private static int FindWhitespaceIndex( ReadOnlySpan<char> lineSpan )
        {
            int idx = 0;
            while (idx < lineSpan.Length && !char.IsWhiteSpace(lineSpan[idx])) idx++;
            return idx;
        }

        private static ReadOnlySpan<char> GetBeginCoverageFlag( string line )
        {
            // try to find #pragma (before pragma may be only whitespace)
            ReadOnlySpan<char> lineSpan = line.AsSpan().TrimStart();
            if (lineSpan.StartsWith(PRAGMA_LINE.AsSpan()))
            {
                int idx = FindNotWhitespaceIndex(lineSpan, PRAGMA_LINE.Length);
                return lineSpan.Slice(idx);
            }

            // try to find single-line comment (before single-line comment may be everything)
            int commentIdx = line.IndexOf(DOUBLE_FORWARD_SLASH_LINE);
            if (commentIdx >= 0)
            {
                lineSpan = line.AsSpan().Slice(commentIdx);
                int idx = FindNotWhitespaceIndex(lineSpan, DOUBLE_FORWARD_SLASH_LINE.Length);
                return lineSpan.Slice(idx);
            }

            // prefix not found
            return ReadOnlySpan<char>.Empty;
        }

        private static LineType GetLineType( string line )
        {
            ReadOnlySpan<char> coverageBeginValue = GetBeginCoverageFlag(line);
            if (!coverageBeginValue.IsEmpty)
            {
                int coverageValueEndIdx = FindWhitespaceIndex(coverageBeginValue);
                ReadOnlySpan<char> value = coverageBeginValue.Slice(0, coverageValueEndIdx);

                if (IsCoverageFlag(value, ENABLE_COVERAGE))
                {
                    return LineType.ENABLE_COVERAGE;
                }
                if (IsCoverageFlag(value, DISABLE_COVERAGE))
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
                        data.Remove(i);
                    }
                    else
                    {
                        // Update data accordingly:
                        if (!enabled)
                        {
                            data.Remove(i);
                        }
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
