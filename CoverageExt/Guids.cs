// Guids.cs
// MUST match guids.h
using System;

namespace NubiloSoft.CoverageExt
{
    static class GuidList
    {
        public const string guidCoverageExtPkgString = "3D61029D-1E2F-4471-93CA-18FDB2F50CD9";
        public const string guidCoverageExtCmdSetString = "E660CA32-95F9-4934-B485-ABA39170A77D";
        public const string guidProjectSpecificMenuCmdSetString = "CAA6399C-8807-43AB-A999-2CA0DB82A56C";
        public const string guidFileSpecificMenuCmdSetString = "0D8FDCC0-98E9-45AD-915A-D3A3BF5DE720";

        public static readonly Guid guidCoverageExtCmdSet = new Guid(guidCoverageExtCmdSetString);
        public static readonly Guid guidProjectSpecificMenuCmdSet = new Guid(guidProjectSpecificMenuCmdSetString);
        public static readonly Guid guidFileSpecificMenuCmdSet = new Guid(guidFileSpecificMenuCmdSetString);
    };
}