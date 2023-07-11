#ifndef EELSTDOUTEXTENSION_H
#define EELSTDOUTEXTENSION_H

typedef void (*stdOutHandler)(const char*, void*);


extern void setPrintfStdOutHandler(stdOutHandler funcPtr, void* userData);
extern int isPrintfStdOutHandlerSet();

#endif // EELSTDOUTEXTENSION_H
