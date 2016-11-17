using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using NubiloSoft.CoverageExt.Data;

namespace NubiloSoft.CoverageExt.Native
{
    public class NativeData : ICoverageData
    {
        private Dictionary<string, BitVector> lookup = new Dictionary<string, BitVector>();

        public BitVector GetData(string filename)
        {
            filename = filename.Replace('/', '\\').ToLower();

            BitVector result = null;
            lookup.TryGetValue(filename, out result);
            return result;
        }

        public DateTime FileDate { get; set; }

        public IEnumerable<Tuple<string, int, int>> Overview()
        {
            foreach (var kv in lookup)
            {
                int covered = 0;
                int uncovered = 0;
                foreach (var item in kv.Value.Enumerate())
                {
                    if (item.Value)
                    {
                        ++covered;
                    }
                    else
                    {
                        ++uncovered;
                    }
                }

                yield return new Tuple<string, int, int>(kv.Key, covered, uncovered);
            }
        }

        public NativeData(string filename)
        {
            // Get file date (for modified checks)
            FileDate = new System.IO.FileInfo(filename).LastWriteTimeUtc;

            // Read file:
            using (var sr = new System.IO.StreamReader(filename))
            {
                string name;
                while ((name = sr.ReadLine()) != null)
                {
                    if (name.StartsWith("FILE: "))
                    {
                        string currentFile = name.Substring("FILE: ".Length);
                        string cov = sr.ReadLine();

                        if (cov.StartsWith("RES: "))
                        {
                            cov = cov.Substring("RES: ".Length);

                            BitVector currentVector = new Data.BitVector();

                            for (int i = 0; i < cov.Length; ++i)
                            {
                                char c = cov[i];
                                if (c == 'c')
                                {
                                    currentVector.Set(i + 1, true);
                                }
                                else if (c == 'u' || c == 'p')
                                {
                                    currentVector.Set(i + 1, false);
                                }
                            }

                            lookup.Add(currentFile.ToLower(), currentVector);
                        }
                    }
                    // otherwise: ignore
                }
            }
        }
    }
}
