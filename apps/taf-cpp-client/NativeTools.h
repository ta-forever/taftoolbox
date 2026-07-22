#pragma once

#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

// Helpers for launching the external native tools on both platforms, mirroring
// the java client's resolution rules (TotalAnnihilationService/MapTool/PosixUidService):
// tools with a native linux build (faf-uid, maptool, gpgnet4ta) drop the .exe
// suffix on linux; windows-only tools (talauncher) keep the .exe and run under
// wine, which accepts unix-style absolute paths in their arguments as-is.
namespace NativeTools
{
    inline QString exeName(const QString& base)
    {
#ifdef Q_OS_WIN
        return base + ".exe";
#else
        return base;
#endif
    }

    // For windows-only tools: returns the program to pass to QProcess::start,
    // prepending the exe path to args when wrapping with wine.
    inline QString wineWrap(const QString& exePath, QStringList& args)
    {
#ifdef Q_OS_WIN
        return exePath;
#else
        args.prepend(exePath);
        return "wine";
#endif
    }
}
