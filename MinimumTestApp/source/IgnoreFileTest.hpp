#include <iostream>

namespace IgnoreFileTest
{
    void TestPrintDigitCpp(int value);

    inline void TestPrintDigitHpp(int value)
    {
        std::cout << "Writing in the hpp file: " << value << std::endl;
    }
}