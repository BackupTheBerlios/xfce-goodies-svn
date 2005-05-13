#ifndef __MY_TRACE_H
#define __MY_TRACE_H

#ifndef TRACE
#ifdef DEBUG
#define TRACE g_warning
#else
static void notrace(char const *format, ...)
{
}
#define TRACE notrace
#endif
#endif

#endif
