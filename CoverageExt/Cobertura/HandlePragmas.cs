using NubiloSoft.CoverageExt.Data;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NubiloSoft.CoverageExt.Cobertura
{
    /// <summary>
    /// This class post-processes the coverage results. We basically scan each file for "#pragma EnableCodeCoverage" 
    /// and "#pragma DisableCodeCoverage" and if we encounter this, we'll update the bit vector.
    /// </summary>
    public class HandlePragmas
    {
        public static void Update(string filename, BitVector data)
        {
            bool enabled = true;

            string[] lines = File.ReadAllLines(filename);
            for (int i = 0; i < lines.Length; ++i)
            {
                string line = lines[i];
                int idx = line.IndexOf("#pragma");
                if (idx >= 0)
                {
                    idx += "#pragma ".Length;
                    string t = line.Substring(idx).TrimStart();
                    if (t.StartsWith("EnableCodeCoverage"))
                    {
                        enabled = true;
                    }
                    else if (t.StartsWith("DisableCodeCoverage"))
                    {
                        enabled = false;
                    }
                }

                // Update data accordingly:
                if (!enabled)
                {
                    data.Remove(i);
                }
            }
        }
    }
}
