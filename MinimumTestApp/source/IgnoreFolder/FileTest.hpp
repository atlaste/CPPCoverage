#include <iostream>
#include <string>

namespace IgnoreFolder::FileTest
{
    void TestPrintStringCpp(const std::string& value);

    inline void TestPrintStringHpp(const std::string& value)
    {
      std::cout << "Writing in the ignore hpp file: " << value << std::endl;
    }
}