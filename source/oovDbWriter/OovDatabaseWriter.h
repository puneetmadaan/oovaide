/*
 * OovDatabaseWriter.h
 * Created on: September 25, 2015
 * \copyright 2015 DCBlaha.  Distributed under the GPL.
 */
#ifndef OOV_DATABASE_WRITER_H
#define OOV_DATABASE_WRITER_H

#include <gmodule.h>


#if defined(SHARED_LIBRARY)
#  define SHAREDSHARED_EXPORT G_MODULE_EXPORT
#else
#  define SHAREDSHARED_EXPORT G_MODULE_IMPORT
#endif


extern "C"
{
SHAREDSHARED_EXPORT bool OpenDb(char const *projectDir, void const *modelData);
/// typeIndex is updated to the last successfully written index.
SHAREDSHARED_EXPORT bool WriteDb(int passIndex, int &typeIndex, int maxTypesPerTransaction);
SHAREDSHARED_EXPORT bool WriteDbComponentTypes(void const *compTypesFile);
SHAREDSHARED_EXPORT char const *GetLastError();
SHAREDSHARED_EXPORT void CloseDb();
}

#endif
