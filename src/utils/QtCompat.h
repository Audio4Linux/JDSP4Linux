#ifndef QTCOMPAT_H
#define QTCOMPAT_H

#include <QMetaType>
#include <QVariant>
#include <QLibraryInfo>

namespace QtCompat
{
    inline int variantTypeId(const QVariant& variant) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        return (int)variant.type();
#else
        return variant.typeId();
#endif
    };

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    inline QString libraryPath(QLibraryInfo::LibraryLocation p) {
        return QLibraryInfo::location(p);
    }
#else
    inline QString libraryPath(QLibraryInfo::LibraryPath p) {
        return QLibraryInfo::path(p);
    }
#endif

};

#endif // QTCOMPAT_H
