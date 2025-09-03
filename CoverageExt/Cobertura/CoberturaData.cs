using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using NubiloSoft.CoverageExt.Data;
using NubiloSoft.CoverageExt.Native;

namespace NubiloSoft.CoverageExt.Cobertura
{
    public class CoberturaData : NativeData
    {
        private string source = null;

        private void PostProcess(string filename)
        {
            filename = filename.Replace('/', '\\').ToLower();
            int idx = filename.IndexOf('\\');
            string basefolder = (idx >= 0)?filename.Substring(0, idx+1):"\\";

            foreach (var item in lookup)
            {
                string file = item.Key;
                file = file.Replace('/', '\\').ToLower();
                if (!file.StartsWith("\\")) { file = basefolder + file; }

                HandlePragmas.Update(file, item.Value.vector);
            }
        }

        public new void Parsing(string filename, string solutionDir)
        {
            // Start initializing the data
            source = null;
            lookup = new Dictionary<string, FileCoverageData>();

            // Current vector
            FileCoverageData current = null;

            // Get file date (for modified checks)
            FileDate = new System.IO.FileInfo(filename).LastWriteTimeUtc;

            // State machines are simple and fast.
            State state = State.XML;
            using (var sr = new System.IO.StreamReader(filename))
            {
                using (var xr = new XmlTextReader(sr))
                {
                    while (xr.Read())
                    {
                        switch (xr.NodeType)
                        {
                            case XmlNodeType.Element:
                                switch (xr.Name)
                                {
                                    case "coverage":
                                        if (!xr.IsEmptyElement && state == State.XML) { state = State.Coverage; }
                                        break;

                                    case "packages":
                                        if (!xr.IsEmptyElement && state == State.Coverage) { state = State.Packages; }
                                        break;

                                    case "package":
                                        if (!xr.IsEmptyElement && state == State.Packages) { state = State.Package; }
                                        break;

                                    case "classes":
                                        if (!xr.IsEmptyElement && state == State.Package) { state = State.Classes; }
                                        break;

                                    case "class":
                                        if (!xr.IsEmptyElement && state == State.Classes)
                                        {
                                            state = State.Class;
                                            string file = xr.GetAttribute("filename").Trim().Replace('/', '\\');
                                            if (!file.StartsWith("\\"))
                                            {
                                                file = "\\" + file;
                                            }

                                            if (!lookup.TryGetValue(file.ToLower(), out current))
                                            {
                                                current.vector = new BitVector();
                                                lookup.Add(file.ToLower(), current);
                                            }
                                        }
                                        break;

                                    case "lines":
                                        if (!xr.IsEmptyElement && state == State.Class) { state = State.Lines; }
                                        break;

                                    case "line":
                                        if (state == State.Lines)
                                        {
                                            string num = xr.GetAttribute("number").Trim();
                                            string hits = xr.GetAttribute("hits").Trim();

                                            if (hits == "0")
                                            {
                                                current.vector.Set(int.Parse(num) - 1, false);
                                            }
                                            else
                                            {
                                                current.vector.Set(int.Parse(num) - 1, true);
                                            }
                                        }
                                        break;

                                    case "sources":
                                        if (state == State.Coverage) { state = State.Sources; }
                                        break;

                                    case "source":
                                        if (state == State.Sources) { state = State.Source; }
                                        break;
                                }
                                break;

                            case XmlNodeType.EndElement:
                                switch (xr.Name)
                                {
                                    case "coverage":
                                        if (state == State.Coverage) { state = State.XML; }
                                        break;

                                    case "packages":
                                        if (state == State.Packages) { state = State.Coverage; }
                                        break;

                                    case "package":
                                        if (state == State.Package) { state = State.Packages; }
                                        break;

                                    case "classes":
                                        if (state == State.Classes) { state = State.Package; }
                                        break;

                                    case "class":
                                        if (state == State.Class)
                                        {
                                            state = State.Classes;
                                            current.vector.Finish();
                                            current.vector = null;
                                        }
                                        break;

                                    case "lines":
                                        if (state == State.Lines) { state = State.Class; }
                                        break;

                                    case "sources":
                                        if (state == State.Sources) { state = State.Coverage; }
                                        break;

                                    case "source":
                                        if (state == State.Source) { state = State.Sources; }
                                        break;

                                }
                                break;

                            case XmlNodeType.Text:
                                if (state == State.Source)
                                {
                                    source = xr.Value.ToLower().Trim();
                                }
                                break;
                        }
                    }
                }
            }

            if (state != State.XML)
            {
                throw new Exception("Error during parsing; expected close to correspond to our open tags");
            }

            PostProcess(filename);
        }
    }
}
