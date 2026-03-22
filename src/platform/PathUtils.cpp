// PathUtils.cpp — Linux implementations of path utility functions
// These are defined in GUIUtils.cpp for macOS/Windows but that file
// depends on VSTGUI, so we provide standalone Linux implementations.

#include <cstring>
#include <libgen.h>

void getFileNameParentPath(const char *path, char *out, int maxLen)
{
    // dirname() may modify input, so copy first
    char *tmp = new char[maxLen];
    strncpy(tmp, path, maxLen - 1);
    tmp[maxLen - 1] = '\0';
    char *dir = dirname(tmp);
    strncpy(out, dir, maxLen - 1);
    out[maxLen - 1] = '\0';
    delete[] tmp;
}

void getFileNameDeletingPathExt(const char *path, char *out, int maxLen)
{
    // Get basename without extension
    char *tmp = new char[maxLen];
    strncpy(tmp, path, maxLen - 1);
    tmp[maxLen - 1] = '\0';
    char *base = basename(tmp);
    char *dot = strrchr(base, '.');
    if (dot) {
        int len = static_cast<int>(dot - base);
        if (len >= maxLen) len = maxLen - 1;
        strncpy(out, base, len);
        out[len] = '\0';
    } else {
        strncpy(out, base, maxLen - 1);
        out[maxLen - 1] = '\0';
    }
    delete[] tmp;
}

void getFileNameExt(const char *path, char *out, int maxLen)
{
    const char *dot = strrchr(path, '.');
    if (dot && dot != path) {
        strncpy(out, dot + 1, maxLen - 1);
        out[maxLen - 1] = '\0';
    } else {
        out[0] = '\0';
    }
}
