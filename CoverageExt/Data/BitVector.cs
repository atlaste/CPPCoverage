using System;
using System.Collections.Generic;

namespace NubiloSoft.CoverageExt.Data
{
    public class BitVector
    {
        private byte[] data = new byte[16];

        public int TotalLines { get; set; }

        private int LastIndex
        {
            get
            {
                int result = (data.Length * 4) - 1;
                while (result >= 0 && !IsFound(result))
                {
                    result--;
                }
                return result;
            }
        }

        public int CodeCount
        {
            get
            {
                var result = 0;
                var lastIndex = LastIndex;
                for (int index = 0; index <= lastIndex; index++)
                {
                    if (IsFound(index))
                        result++;
                }
                return result;
            }
        }

        public void Set(int index, bool value)
        {
            Ensure(index + 1);

            int byteIndex = index >> 2;
            int bitIndex = index & 0x3;

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

        private void Ensure(int index)
        {
            int byteIndex = index >> 2;

            if (byteIndex >= data.Length)
            {
                Array.Resize(ref data, byteIndex * 2);
            }
        }

        private byte GetValue(int index)
        {
            int byteIndex = index >> 2;
            return (byteIndex < data.Length) ? data[byteIndex] : (byte)0;
        }

        public bool IsSet(int index)
        {
            int bitIndex = index & 0x3;
            var value = GetValue(index);

            return ((value >> bitIndex) & 0x01) != 0;
        }

        public bool IsFound(int index)
        {
            int bitIndex = index & 0x3;
            var value = GetValue(index);

            return ((value >> bitIndex) & 0x10) != 0;
        }

        public void Remove(int index)
        {
            var b = GetValue(index);
            if (b == 0) return;

            int byteIndex = index >> 2;
            int bitIndex = index & 0x3;

            byte mask = (byte)(0xFF ^ (0x11 << bitIndex));
            data[byteIndex] = (byte)(mask & b);
        }

        public IEnumerable<KeyValuePair<int, bool>> Enumerate()
        {
            var lastIndex = LastIndex;
            for (int index = 0; index <= lastIndex; ++index)
            {
                if (IsFound(index))
                {
                    bool found = IsSet(index);
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
