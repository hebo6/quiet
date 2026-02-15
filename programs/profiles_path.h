#ifndef PROFILES_PATH_H
#define PROFILES_PATH_H

#include <string.h>
#include <unistd.h>
#include <limits.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

static const char *get_profiles_path(void) {
    static char path[PATH_MAX];
    char exe_path[PATH_MAX];

#ifdef __linux__
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len == -1) return QUIET_PROFILES_LOCATION;
    exe_path[len] = '\0';
#elif defined(__APPLE__)
    uint32_t size = sizeof(exe_path);
    if (_NSGetExecutablePath(exe_path, &size) != 0)
        return QUIET_PROFILES_LOCATION;
    char resolved[PATH_MAX];
    if (realpath(exe_path, resolved))
        strncpy(exe_path, resolved, sizeof(exe_path) - 1);
#else
    return QUIET_PROFILES_LOCATION;
#endif

    char *last_slash = strrchr(exe_path, '/');
    if (!last_slash) return QUIET_PROFILES_LOCATION;
    *last_slash = '\0';

    snprintf(path, sizeof(path), "%s/quiet-profiles.json", exe_path);

    if (access(path, R_OK) == 0) return path;

    return QUIET_PROFILES_LOCATION;
}

#endif
