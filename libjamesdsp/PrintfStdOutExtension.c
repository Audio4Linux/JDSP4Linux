#include "PrintfStdOutExtension.h"
#include <stdarg.h>

static stdOutHandler _printfStdOutHandlerPtr = 0;
static void* _printfStdOutHandlerUserPtr = 0;

int redirected_printf(const char * format, ...) {
    // Make formatted string.
    char* outstr = 0;
    va_list ap;
    va_start(ap, format);
    int result = vasprintf(&outstr, format, ap);
    va_end(ap);
    if(result < 0) // Error occurred and `outstr` undefined if result < 0.
        return result;

    if(_printfStdOutHandlerPtr != 0)
    {
        _printfStdOutHandlerPtr(outstr, _printfStdOutHandlerUserPtr);
    }

    free(outstr);
    return result;
}

void __android_log_print(int severity, const char* tag, const char* msg) {
    char *s;
    if (asprintf(&s, "%s: %s", tag, msg) > 0 && s != 0)
    {
        if(_printfStdOutHandlerPtr != 0)
        {
            _printfStdOutHandlerPtr(s, _printfStdOutHandlerUserPtr);
        }
        free(s);
    }
}

void setPrintfStdOutHandler(stdOutHandler funcPtr, void* userData)
{
    _printfStdOutHandlerPtr = funcPtr;
    _printfStdOutHandlerUserPtr = userData;
}

int isPrintfStdOutHandlerSet()
{
    return _printfStdOutHandlerPtr != 0;
}
