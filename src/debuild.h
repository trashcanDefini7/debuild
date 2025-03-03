#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#define DEB_GLOBAL_PREFIX inline

#ifdef _WIN32
#define DEB_NEWLINE "\r\n"
#else
#define DEB_NEWLINE "\n"
#endif

typedef FILE* Deb_Log_Stream;

typedef struct
{
    char* data;
    size_t length;
} Deb_StringView;

typedef enum
{
    Deb_Log_Level_Error,
    Deb_Log_Level_Warn,
    Deb_Log_Level_Info
} Deb_Log_Level;

DEB_GLOBAL_PREFIX Deb_Log_Level deb_log_level;
DEB_GLOBAL_PREFIX Deb_Log_Stream deb_log_stream_error;
DEB_GLOBAL_PREFIX Deb_Log_Stream deb_log_stream_warn;
DEB_GLOBAL_PREFIX Deb_Log_Stream deb_log_stream_info;

void deb_log_init();
void deb_log(Deb_Log_Stream, const char*, const char*, ...);
void deb_log_error(const char*, ...);
void deb_log_warn(const char*, ...);
void deb_log_info(const char*, ...);

#define deb_cmd_append(cmd, ...) \
    deb_cmd_append_impl(cmd, sizeof((const char* []){__VA_ARGS__}) / sizeof(const char*), __VA_ARGS__)

typedef struct
{
    size_t args_capacity;
    size_t args_count;
    Deb_StringView* args;
} Deb_Cmd;

void deb_init();
bool deb_cmd_append_impl(Deb_Cmd*, size_t, ...);
bool deb_cmd_execute(Deb_Cmd*);

#ifdef DEBUILD_IMPLEMENTATION
#undef DEBUILD_IMPLEMENTATION

void deb_log_init()
{
    deb_log_level = Deb_Log_Level_Error;

    deb_log_stream_error = stderr;
    deb_log_stream_warn = stdout;
    deb_log_stream_info = stdout;
}

void deb_log(Deb_Log_Stream stream, const char* prefix, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    fprintf(stream, prefix);
    fprintf(stream, format, args);
    fprintf(stream, DEB_NEWLINE);

    va_end(args);
}

void deb_log_error(const char* format, ...)
{
    if (deb_log_level <= Deb_Log_Level_Error)
    {
        va_list args;
        va_start(args, format);

        deb_log(deb_log_stream_error, "[ERROR] ", format, args);

        va_end(args);
    }
}

void deb_log_warn(const char* format, ...)
{
    if (deb_log_level <= Deb_Log_Level_Warn)
    {
        va_list args;
        va_start(args, format);

        deb_log(deb_log_stream_warn, "[WARN] ", format, args);

        va_end(args);
    }
}

void deb_log_info(const char* format, ...)
{
    if (deb_log_level <= Deb_Log_Level_Info)
    {
        va_list args;
        va_start(args, format);

        deb_log(deb_log_stream_info, "[INFO] ", format, args);

        va_end(args);
    }
}

void deb_init()
{
    deb_log_init();
}

bool deb_cmd_append_impl(Deb_Cmd* cmd, size_t args_count, ...)
{
    if (cmd == NULL)
    {
        deb_log_error("cmd is NULL in deb_cmd_append");
        return false;
    }

    va_list args;
    va_start(args, args_count);

    size_t new_args_count = cmd->args_count + args_count;

    if (cmd->args_capacity < new_args_count)
    {
        cmd->args_capacity = new_args_count / 2 + new_args_count;
        cmd->args = (Deb_StringView*)realloc(cmd->args, cmd->args_capacity * sizeof(Deb_StringView));
    }

    for (size_t i = 0; i < args_count; i++)
    {
        const char* src = va_arg(args, const char*);

        if (!src)
        {
            deb_log_error("Argument is NULL in deb_cmd_append");
            va_end(args);
            return false;
        }

        Deb_StringView* arg = &cmd->args[cmd->args_count++];

        arg->length = strlen(src);
        arg->data = (char*)calloc(arg->length + 1, sizeof(char));

        strcpy(arg->data, src);
    }

    va_end(args);
    return true;
}

bool deb_cmd_execute(Deb_Cmd* cmd)
{
    if (cmd == NULL)
    {
        deb_log_error("cmd is NULL in deb_cmd_execute");
        return false;
    }

    size_t bytes_count = 0;

    for (int i = 0; i < cmd->args_count; i++)
        bytes_count += cmd->args[i].length + 1;

    char* input = (char*)calloc(bytes_count, sizeof(char));
    int j = 0;

    for (int i = 0; i < cmd->args_count; i++)
    {
        Deb_StringView* arg = &cmd->args[i];

        for (int k = 0; arg->data[k] != '\0'; k++)
            input[j++] = arg->data[k];

        input[j++] = ' ';
    }

    input[j] = '\0';

    if (system(input) != 0)
    {
        deb_log_error("Couldn't execute: %s", input);
        return false;
    }

    return true;
}

#endif
