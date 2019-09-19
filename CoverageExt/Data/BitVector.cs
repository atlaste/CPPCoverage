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
            Ensure(index + 1);

            int byteIndex = index >> 2;
            int bitIndex = (index & 0x3);

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

        public void Ensure(int index)
        {
            int byteIndex = index >> 2;

            if (byteIndex >= data.Length)
            {
                Array.Resize(ref data, byteIndex * 2);
            }
        }

        public bool IsSet(int index)
        {
            int byteIndex = index >> 2;
            int bitIndex = (index & 0x3);

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
            int byteIndex = index >> 2;
            int bitIndex = (index & 0x3);

            if (byteIndex >= data.Length)
            {
                return false;
            }
            else
            {
                return ((data[byteIndex] >> bitIndex) & 0x10) != 0;
            }
        }

        public void Remove(int index)
        {
            int byteIndex = index >> 2;
            int bitIndex = (index & 0x3);

            if (byteIndex < data.Length)
            {
                byte b = data[byteIndex];
                byte mask = (byte)(0xFF ^ (0x11 << bitIndex));
                data[byteIndex] = (byte)(mask & b);
            }
        }

        public IEnumerable<KeyValuePair<int, bool>> Enumerate()
        {
            for (int index = 0; index < data.Length * 4; ++index)
            {
                int byteIndex = index >> 2;
                int bitIndex = (index & 0x3);

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
