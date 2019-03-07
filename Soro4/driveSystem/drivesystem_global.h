#ifndef DRIVESYSTEM_GLOBAL_H
#define DRIVESYSTEM_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(DRIVESYSTEM_LIBRARY)
#  define DRIVESYSTEMSHARED_EXPORT Q_DECL_EXPORT
#else
#  define DRIVESYSTEMSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // DRIVESYSTEM_GLOBAL_H
