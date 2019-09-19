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
        private Dictionary<string, Tuple<BitVector, ProfileVector>> lookup = new Dictionary<string, Tuple<BitVector, ProfileVector>>();

        public Tuple<BitVector, ProfileVector> GetData(string filename)
        {
            filename = filename.Replace('/', '\\').ToLower();

            Tuple<BitVector, ProfileVector> result = null;
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
                foreach (var item in kv.Value.Item1.Enumerate())
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
                string name = sr.ReadLine();
                while (name != null)
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
                                if (c == 'c' || c == 'p')
                                {
                                    currentVector.Set(i, true);
                                }
                                else if (c == 'u')
                                {
                                    currentVector.Set(i, false);
                                }
                                else
                                {
                                    currentVector.Ensure(i + 1);
                                }
                            }

                            ProfileVector currentProfile = new Data.ProfileVector(currentVector.Count);

                            string prof = sr.ReadLine();
                            if (prof != null && prof.StartsWith("PROF: "))
                            {
                                prof = prof.Substring("PROF: ".Length);
                                int line = 0;

                                for (int i = 0; i < prof.Length;)
                                {
                                    int deep = 0;
                                    while (i < prof.Length && prof[i] != ',')
                                    {
                                        char c = prof[i];
                                        deep = deep * 10 + (c - '0');
                                        ++i;
                                    }
                                    ++i;

                                    int shallow = 0;
                                    while (i < prof.Length && prof[i] != ',')
                                    {
                                        char c = prof[i];
                                        shallow = shallow * 10 + (c - '0');
                                        ++i;
                                    }
                                    ++i;

                                    currentProfile.Set(line, deep, shallow);
                                    ++line;
                                }
                            }
                            else
                            {
                                name = prof;
                                lookup.Add(currentFile.ToLower(), new Tuple<BitVector, ProfileVector>(currentVector, currentProfile));
                                continue;
                            }

                            lookup.Add(currentFile.ToLower(), new Tuple<BitVector, ProfileVector>(currentVector, currentProfile));
                        }
                    }
                    // otherwise: ignore; grab next line

                    name = sr.ReadLine();
                }
            }
        }
    }
}
