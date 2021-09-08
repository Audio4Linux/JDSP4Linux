#ifndef TEMPLATEEXTENSION_H
#define TEMPLATEEXTENSION_H
template <typename T> const char *typeName()
{
    #ifdef _MSC_VER
    return __FUNCSIG__;
    #else
    return __PRETTY_FUNCTION__;
    #endif
}
#endif // TEMPLATEEXTENSION_H
