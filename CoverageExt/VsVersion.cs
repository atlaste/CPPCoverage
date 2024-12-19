using System;
using System.Diagnostics;
using System.IO;

namespace NubiloSoft.CoverageExt
{
    public static class VsVersion
    {
        static readonly object mLock = new object();
        static Version mVsVersion;

        public static Version FullVersion
        {
            get
            {
                lock (mLock)
                {
                    if (mVsVersion == null)
                    {
                        try
                        {
                            string path = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "msenv.dll");
                            FileVersionInfo fvi = FileVersionInfo.GetVersionInfo(path);
                            string verName = fvi.ProductVersion;

                            for (int i = 0; i < verName.Length; i++)
                            {
                                if (!char.IsDigit(verName, i) && verName[i] != '.')
                                {
                                    verName = verName.Substring(0, i);
                                    break;
                                }
                            }
                            mVsVersion = new Version(verName);
                        }
                        catch (FileNotFoundException e)
                        {
                            mVsVersion = new Version(0, 0); // Not running inside Visual Studio!
                        }
                    }
                }

                return mVsVersion;
            }
        }

        public static bool Vs2022OrLater
        {
            get { return FullVersion >= new Version(17, 0); }
        }
    }
}
