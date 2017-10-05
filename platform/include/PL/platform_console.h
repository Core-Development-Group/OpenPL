/*
This is free and unencumbered software released into the public domain.

Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.

In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

For more information, please refer to <http://unlicense.org>
*/

#pragma once

#include <PL/platform.h>
#include <PL/platform_math.h>

enum {
    PL_VAR_FLOAT,
    PL_VAR_INTEGER,
    PL_VAR_STRING,
    PL_VAR_BOOLEAN,
};

typedef struct PLConsoleVariable {
    const char *var, *default_value;

    unsigned int type;

    void(*Callback)(unsigned int argc, char *argv[]);

    char description[512];

    /////////////////////////////

    char value[1024];
} PLConsoleVariable;

PL_EXTERN_C

void plGetConsoleVariables(PLConsoleVariable *** const vars, size_t * const num_vars);
PLConsoleVariable *plGetConsoleVariable(const char *name);

void plRegisterConsoleVariables(PLConsoleVariable vars[], unsigned int num_vars);

#define plAddConsoleVariable(NAME, ...) \
    PLConsoleVariable NAME = { #NAME, __VA_ARGS__ }; \
    plRegisterConsoleVariables(&NAME, 1);

PL_EXTERN_C_END

/////////////////////////////////////////////////////////////////////////////////////

typedef struct PLConsoleCommand {
    char cmd[24];

    void(*Callback)(unsigned int argc, char *argv[]);

    char description[512];
} PLConsoleCommand;

PL_EXTERN_C

void plGetConsoleCommands(PLConsoleCommand *** const cmds, size_t * const num_cmds);
PLConsoleCommand *plGetConsoleCommand(const char *name);

PL_EXTERN_C_END

/////////////////////////////////////////////////////////////////////////////////////

PL_EXTERN_C

PL_EXTERN void plSetupConsole(unsigned int num_instances);

PL_EXTERN void plSetConsoleColour(unsigned int id, PLColour colour);

PL_EXTERN void plParseConsoleString(const char *string);

PL_EXTERN void plShowConsole(bool show);
PL_EXTERN void plDrawConsole(void);

PL_EXTERN_C_END