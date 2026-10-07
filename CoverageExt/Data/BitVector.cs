using System;
using System.Collections.Generic;

namespace NubiloSoft.CoverageExt.Data
{
    public class BitVector
    {
        private const byte VALUE_MASK = 0x11;

        private const byte VALUE_COVERED = 0x11;
        private const byte VALUE_PARTIALLY = 0x10;
        private const byte VALUE_UNCOVERED = 0x01;

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

        public void Set(int index, CoverageState state)
        {
            Ensure(index + 1);

            int byteIndex = index >> 2;
            int bitIndex = index & 0x3;

            byte value = 0;
            switch (state)
            {
                case CoverageState.Covered: value = VALUE_COVERED; break;
                case CoverageState.Partially: value = VALUE_PARTIALLY; break;
                case CoverageState.Uncovered: value = VALUE_UNCOVERED; break;
                default: break;
            }

            byte b = (byte)(value << bitIndex);
            data[byteIndex] |= b;
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

        public CoverageState GetCoverageState(int index)
        {
            int bitIndex = index & 0x3;
            var value = GetValue(index);
            var valueState = (value >> bitIndex) & VALUE_MASK;

            switch (valueState)
            {
                case VALUE_COVERED: return CoverageState.Covered;
                case VALUE_PARTIALLY: return CoverageState.Partially;
                case VALUE_UNCOVERED: return CoverageState.Uncovered;
                default: return CoverageState.Irrelevant;
            }
        }

        private bool IsFound(int index)
        {
            int bitIndex = index & 0x3;
            var value = GetValue(index);

            return ((value >> bitIndex) & VALUE_MASK) != 0;
        }

        public void Remove(int index)
        {
            var b = GetValue(index);
            if (b == 0) return;

            int byteIndex = index >> 2;
            int bitIndex = index & 0x3;

            byte mask = (byte)(0xFF ^ (VALUE_MASK << bitIndex));
            data[byteIndex] = (byte)(mask & b);
        }

        public IEnumerable<KeyValuePair<int, CoverageState>> Enumerate()
        {
            var lastIndex = LastIndex;
            for (int index = 0; index <= lastIndex; ++index)
            {
                if (IsFound(index))
                {
                    CoverageState state = GetCoverageState(index);
                    yield return new KeyValuePair<int, CoverageState>(index, state);
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
