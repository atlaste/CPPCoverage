﻿using NubiloSoft.CoverageExt.Data;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Xml;

namespace NubiloSoft.CoverageExt.Native
{
    public class NativeV2Data : ICoverageData
    {
        public class NativeV2CoverageData : IFileCoverageData
        {
            static UInt16 maskCount = 0x3FFF;       ///< Count access of this line
            static UInt16 maskIsCode = 0x8000;
            static UInt16 maskIsPartial = 0x4000;

            public FileCoverageStats stats;
            ushort[] _lines;

            internal void addCoverage(string encodedString)
            {
                byte[] data = Convert.FromBase64String(encodedString);
                Debug.Assert(data.Length % 2 == 0);

                _lines = new ushort[data.Length / 2];
                for (UInt32 objIndex = 0; objIndex < _lines.Length; ++objIndex)
                {
                    UInt32 id = objIndex * sizeof(UInt16);
                    _lines[objIndex] = (ushort)((data[id + 1] << 8) + data[id]);
                }
            }
            // Check FileCoverageV2.h : it's must be the same
            // |    15   |     14     | 13 ---- 0 | 
            // | is code | is partial |   count   |

            uint IFileCoverageData.count(uint idLine)
            {
                Debug.Assert(idLine < _lines.Count());
                return (uint)_lines[idLine] & maskCount;
            }

            CoverageState IFileCoverageData.state(uint idLine)
            {
                Debug.Assert(idLine < _lines.Count());
                if ((_lines[idLine] & maskIsCode) == 0)
                {
                    return CoverageState.Irrelevant;
                }
                if ((_lines[idLine] & maskIsPartial) == 0)
                {
                    return (_lines[idLine] & maskCount) > 0 ? CoverageState.Covered : CoverageState.Uncovered;
                }
                else
                    return CoverageState.Partially;
            }
            UInt32 IFileCoverageData.nbLines()
            {
                return (UInt32)_lines.Length;
            }

            ProfileVector IFileCoverageData.profile()
            {
                return null;
            }

            public bool hasCounting()
            {
                return true;
            }
        }

        private Dictionary<string, NativeV2CoverageData> lookup = new Dictionary<string, NativeV2CoverageData>();

        public IFileCoverageData GetData(string filename)
        {
            filename = filename.Replace('/', '\\').ToLower();

            NativeV2CoverageData result = null;
            lookup.TryGetValue(filename, out result);
            return result;
        }

        public DateTime FileDate { get; set; }

        public IEnumerable<Tuple<string, FileCoverageStats>> Overview()
        {
            foreach (var kv in lookup)
            {
                yield return new Tuple<string, FileCoverageStats>(kv.Key, kv.Value.stats);
            }
        }

        public UInt32 nbEntries() => (UInt32)lookup.Count();

        void ICoverageData.Parsing(string filename, string solutionDir)
        {
            // Get file date (for modified checks)
            FileDate = new System.IO.FileInfo(filename).LastWriteTimeUtc;

            XmlDocument xmlDoc = new XmlDocument();
            xmlDoc.Load(filename);
            var xmlEntities = new List<XmlEntity>();

            XmlNodeList filesNodes = xmlDoc.SelectNodes("/CppCoverage/file");
            foreach (XmlNode item in filesNodes)
            {
                if (item.LocalName == "file")
                {
                    string currentFile = item.Attributes["path"].InnerText;
                    // File is valid
                    if (!String.IsNullOrEmpty(currentFile))
                    {
                        // Build full path if needed
                        if (!System.IO.Path.IsPathRooted(currentFile))
                        {
                            currentFile = System.IO.Path.Combine(solutionDir, currentFile);
                        }

                        currentFile = currentFile.Replace('/', '\\').ToLower();

                        // Check file is existing and keep the coverage information
                        if (System.IO.File.Exists(currentFile))
                        {
                            var coverage = new NativeV2CoverageData();
                            coverage.stats = new FileCoverageStats();

                            foreach (XmlNode fileItem in item.ChildNodes)
                            {
                                //Read stat : <stats nbLinesInFile="67" nbLinesOfCode="22" nbLinesCovered="0"/>
                                if (fileItem.LocalName == "stats")
                                {
                                    coverage.stats.lineInsideFile = UInt32.Parse(fileItem.Attributes["nbLinesInFile"].InnerText);
                                    coverage.stats.lineOfCodeFile = UInt32.Parse(fileItem.Attributes["nbLinesOfCode"].InnerText);
                                    coverage.stats.lineCoveredFile = UInt32.Parse(fileItem.Attributes["nbLinesCovered"].InnerText);
                                }
                                //Read coverage :
                                else if (fileItem.LocalName == "coverage")
                                {
                                    coverage.addCoverage(fileItem.InnerText);
                                }
                            }
                            lookup.Add(currentFile, coverage);
                        }
                    }
                }
            }
        }
    }
}
