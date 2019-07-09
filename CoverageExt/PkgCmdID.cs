// PkgCmdID.cs
// MUST match PkgCmdID.h
using System;

namespace NubiloSoft.CoverageExt
{
    static class PkgCmdIDList
    {
        public const uint cmdCoverageReport = 0x0100;
        public const uint cmdCoverageGenerate = 0x0102;
        public const uint cmdCoverageShow = 0x0104;
    };
}