#ifndef EVENTARGS_H
#define EVENTARGS_H

#include <list>

struct ConvolverInfoEventArgs
{
    std::list<float> data;
    int channels;
    int frames;
};

#endif // EVENTARGS_H
