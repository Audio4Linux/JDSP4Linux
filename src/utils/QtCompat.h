#ifndef QTCOMPAT_H
#define QTCOMPAT_H

#include <QMetaType>
#include <QVariant>

class QtCompat
{
public:
    static QMetaType::Type variantTypeId(const QVariant& variant) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        return (QMetaType::Type)variant.type();
#else
        return variant.typeId();
#endif
    };
};

#endif // QTCOMPAT_H
