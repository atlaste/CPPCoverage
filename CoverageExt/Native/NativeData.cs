using System;
using System.Collections.Generic;
using System.Linq;
using NubiloSoft.CoverageExt.Data;

namespace NubiloSoft.CoverageExt.Native
{
    public class NativeData : ICoverageData
    {
        public class FileCoverageData : IFileCoverageData
        {
            public BitVector vector;
            public ProfileVector profile;

            public FileCoverageData(BitVector vector, ProfileVector profile)
            {
                this.vector = vector;
                this.profile = profile;
            }

            uint IFileCoverageData.count(uint idLine)
            {
                return 0;
            }

            CoverageState IFileCoverageData.state(uint idLine)
            {
                return vector.IsFound((int)idLine) ? CoverageState.Covered : CoverageState.Uncovered;
            }

            UInt32 IFileCoverageData.nbLines()
            {
                return (UInt32)vector.Count;
            }

            ProfileVector IFileCoverageData.profile()
            {
                return profile;
            }

            public bool hasCounting()
            {
                return false;
            }
        }

        private Dictionary<string, FileCoverageData> lookup = new Dictionary<string, FileCoverageData>();

        public IFileCoverageData GetData(string filename)
        {
            filename = filename.Replace('/', '\\').ToLower();

            FileCoverageData result = null;
            lookup.TryGetValue(filename, out result);
            return result;
        }

        public DateTime FileDate { get; set; }

        public UInt32 nbEntries() => (UInt32)lookup.Count();

        public IEnumerable<Tuple<string, FileCoverageStats>> Overview()
        {
            foreach (var kv in lookup)
            {
                var stats = new FileCoverageStats();
                foreach (var item in kv.Value.vector.Enumerate())
                {
                    if (item.Value)
                    {
                        ++stats.lineCoveredFile;
                    }
                }
                stats.lineOfCodeFile = (uint)kv.Value.vector.Count;
                stats.lineInsideFile = stats.lineOfCodeFile;

                yield return new Tuple<string, FileCoverageStats>(kv.Key, stats);
            }
        }

        public void Parsing(string filename, string solutionDir)
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
                                lookup.Add(currentFile.ToLower(), new FileCoverageData(currentVector, currentProfile));
                                continue;
                            }

                            lookup.Add(currentFile.ToLower(), new FileCoverageData(currentVector, currentProfile));
                        }
                    }
                    // otherwise: ignore; grab next line

                    name = sr.ReadLine();
                }
            }
        }
    }
}
