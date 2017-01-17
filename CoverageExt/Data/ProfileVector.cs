using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace NubiloSoft.CoverageExt.Data
{
    public class ProfileVector
    {
        public ProfileVector(int count)
        {
            ProfileData = new ushort[count];
        }

        public ushort[] ProfileData;

        public void Set(int line, int deep, int shallow)
        {
            if (line < ProfileData.Length)
            {
                ProfileData[line] = (ushort)(deep << 8 | shallow);
            }
        }

        public Tuple<int, int> Get(int line)
        {
            if (line < ProfileData.Length)
            {
                ushort val = ProfileData[line];
                return new Tuple<int, int>((val >> 8) & 0xFF, val & 0xFF);
            }
            else
            {
                return new Tuple<int, int>(0, 0);
            }
        }
    }
}
