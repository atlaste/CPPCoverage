using System;
using System.Collections.Generic;

namespace NubiloSoft.CoverageExt.Data
{
    public class BitVector
    {
        private byte[] data = new byte[16];

        public int Count { get { return data.Length * 4; } }

        public void Set(int index, bool value)
        {
            int byteIndex = index >> 2;
            int bitIndex = index & 0x3;

            if (byteIndex >= data.Length)
            {
                Array.Resize(ref data, byteIndex * 2);
            }

            if (value)
            {
                byte b = (byte)(0x11 << bitIndex);
                data[byteIndex] |= b;
            }
            else
            {
                byte b = (byte)(0x10 << bitIndex);
                data[byteIndex] |= b;
            }
        }

        public bool IsSet(int index)
        {
            int byteIndex = index / 4;
            int bitIndex = index % 4;

            if (byteIndex >= data.Length)
            {
                return false;
            }
            else
            {
                return ((data[byteIndex] >> bitIndex) & 0x01) != 0;
            }
        }

        public bool IsFound(int index)
        {
            int byteIndex = index / 4;
            int bitIndex = index % 4;

            if (byteIndex >= data.Length)
            {
                return false;
            }
            else
            {
                return ((data[byteIndex] >> bitIndex) & 0x10) != 0;
            }
        }

        public IEnumerable<KeyValuePair<int, bool>> Enumerate()
        {
            for (int index = 0; index < data.Length * 4; ++index)
            {
                int byteIndex = index >> 2;
                int bitIndex = index & 3;

                if (((data[byteIndex] >> bitIndex) & 0x10) != 0)
                {
                    bool found = (((data[byteIndex] >> bitIndex) & 0x01) != 0);
                    yield return new KeyValuePair<int, bool>(index, found);
                }
            }
        }

        public void Finish()
        {
            int i = -1;
            for (i = data.Length - 1; i >= 0 && data[i] == 0; --i)
            {
            }
            ++i;

            Array.Resize(ref data, i);
        }
    }

}
