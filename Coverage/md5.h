
#include <windows.h>
#include <Wincrypt.h>

#include <array>
#include <cassert>
#include <cstdio>
#include <filesystem>
#include <format>
#include <fstream>
#include <span>

class MD5
{
  static const uint32_t MD5Length = 16;

  HCRYPTPROV _hProv = 0;

  struct Hash
  {
    HCRYPTHASH _handle = 0;
    Hash(MD5& md5)
    {
      // Create Hash
      if (!CryptCreateHash(md5._hProv, CALG_MD5, 0, 0, &_handle))
      {
        throw std::runtime_error(std::format("CryptCreateHash failed: {0}", GetLastError()));
      }
    }

    void addData(const std::string& buffer, const std::streamsize readSize) const
    {
      assert(0 < readSize && readSize <= static_cast<std::streamsize>(buffer.size()));
      if (!CryptHashData(_handle, reinterpret_cast<const BYTE*>(buffer.data()), static_cast<DWORD>(readSize), 0))
      {
        throw std::runtime_error(std::format("CryptHashData failed: {0}", GetLastError()));
      }
    }

    std::string computeMd5() const
    {
      std::array<BYTE, MD5::MD5Length> encoding{ { 0 } };

      std::string md5Hash;
      md5Hash.reserve(MD5::MD5Length * 2);

      const std::string rgbDigits("0123456789abcdef");
      DWORD cbHash = static_cast<DWORD>(encoding.size());
      if (!CryptGetHashParam(_handle, HP_HASHVAL, encoding.data(), &cbHash, 0))
      {
        throw std::runtime_error(std::format("CryptHashData failed: {0}", GetLastError()));
      }
      assert(cbHash == encoding.size());

      // To hex
      for (const auto& c : encoding)
      {
        md5Hash.push_back(rgbDigits[c >> 4]);
        md5Hash.push_back(rgbDigits[c & 0xf]);
      }

      return md5Hash;
    }

    ~Hash()
    {
      //Release Hash
      CryptDestroyHash(_handle);
    }
  };

public:

  MD5()
  {
    // Get handle to the crypto provider
    if (!CryptAcquireContext(&_hProv,
                             NULL,
                             NULL,
                             PROV_RSA_FULL,
                             CRYPT_VERIFYCONTEXT))
    {
      throw std::runtime_error(std::format("CryptAcquireContext failed: {0}", GetLastError()));
    }
  }

  std::string encode(const std::filesystem::path& filepath)
  {
    // Create hash system
    Hash hash(*this);

    // Read Buffer of 1k
    std::string buffer;
    buffer.resize(1024 * 1024);
    std::streamsize readBuffer = 0;

    std::ifstream ifs;
    ifs.open(filepath);
    if (ifs.is_open())
    {
      while (ifs.read(buffer.data(), buffer.size()))
      {
        readBuffer = buffer.find('\0');
        hash.addData(buffer, readBuffer == -1 ? buffer.size() : readBuffer);
      }

      readBuffer = buffer.find('\0');
      if (readBuffer > 0)
      {
        hash.addData(buffer, readBuffer);
      }

      return hash.computeMd5();
    }
    else
    {
      return {};
    }
  }

  ~MD5()
  {
    //Release context
    CryptReleaseContext(_hProv, 0);
  }
};