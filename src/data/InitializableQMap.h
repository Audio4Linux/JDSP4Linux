#ifndef INITIALIZABLEQMAP_H
#define INITIALIZABLEQMAP_H
#include <QMap>

template<class Key, class T>
class InitializableQMap :
    public QMap<Key, T>
{
public:
    inline InitializableQMap<Key, T> &operator<< (const QPair<Key, T> &t)
    { this->insert(t.first, t.second); return *this; }
};

#endif // INITIALIZABLEQMAP_H
