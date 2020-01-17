#ifndef _GVFS_NATIVE_H_
#define _GVFS_NATIVE_H_

#include <gglobal.h>
#include <gstdio.h>

#ifdef __cplusplus
extern "C" {
#endif

G_API void gvfs_init();
G_API void gvfs_cleanup();

G_API void gvfs_setZipFile(const char *archiveFile);
G_API void gvfs_setPlayerModeEnabled(int playerMode);
G_API int gvfs_isPlayerModeEnabled();
G_API void gvfs_setZipFiles(const char *apkFile, const char *mainFile, const char *patchFile);
G_API void gvfs_addFile(const char *pathname, int zipFile, size_t startOffset, size_t length);
G_API void gvfs_setCodeKey(const char key[16]);
G_API void gvfs_setAssetsKey(const char key[16]);

#ifdef __cplusplus
}
#endif

#endif
