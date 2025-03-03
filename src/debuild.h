#pragma once

#ifndef _WIN32
#error You can't use this library outside of the Windows
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

/*#ifndef DEB_ASSERT
#include <assert.h>
#define DEB_ASSERT(condition, message) assert((condition) && (message))
#endif*/

#define DEB_GLOBAL_PREFIX static
#define DEB_WIN32_ERROR_BUFFER_LENGTH 256

const char *const deb_get_win32_error();

typedef FILE* Deb_LogStream;

typedef struct
{
    char* data;
    size_t length;
} Deb_StringView;

typedef enum
{
    Deb_LogType_Error,
    Deb_LogType_Warn,
    Deb_LogType_Info
} Deb_LogType;

DEB_GLOBAL_PREFIX Deb_LogType deb_log_level;
DEB_GLOBAL_PREFIX Deb_LogStream deb_log_stream_error;
DEB_GLOBAL_PREFIX Deb_LogStream deb_log_stream_warn;
DEB_GLOBAL_PREFIX Deb_LogStream deb_log_stream_info;

void deb_log_init();
void deb_log(Deb_LogType, const char*, ...);

#define deb_cmd_append(cmd, ...) \
    deb_cmd_append_impl(cmd, sizeof((const char* []){__VA_ARGS__}) / sizeof(const char*), __VA_ARGS__)

typedef struct
{
    size_t args_capacity;
    size_t args_count;
    Deb_StringView* args;
} Deb_Cmd;

void deb_init();
void deb_panic(const char* message);

bool deb_cmd_append_impl(Deb_Cmd*, size_t, ...);
bool deb_cmd_execute(Deb_Cmd*);

bool deb_dir_exists(const char*);
char* deb_dir_list(const char*);

bool deb_file_exists(const char*);
bool deb_file_copy(const char*, const char*);

#ifdef DEBUILD_IMPLEMENTATION
#undef DEBUILD_IMPLEMENTATION

const char *const deb_get_win32_error()
{
    DWORD msg_id = GetLastError();

    if (msg_id == 0)
        return NULL;

    static char buffer[DEB_WIN32_ERROR_BUFFER_LENGTH];

    size_t msg_size = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, msg_id, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buffer, DEB_WIN32_ERROR_BUFFER_LENGTH, NULL);

    return buffer;
}

void deb_log_init()
{
    deb_log_level = Deb_LogType_Error;

    deb_log_stream_error = stderr;
    deb_log_stream_warn = stdout;
    deb_log_stream_info = stdout;
}

void deb_log(Deb_LogType type, const char* format, ...)
{
    if (type < deb_log_level)
        return;

    const char* prefix;
    Deb_LogStream* stream;

    switch (type)
    {
    case Deb_LogType_Error:
    {
        prefix = "[ERROR] ";
        stream = &deb_log_stream_error;
    }
    break;

    case Deb_LogType_Warn:
    {
        prefix = "[WARN] ";
        stream = &deb_log_stream_warn;
    }
    break;

    case Deb_LogType_Info:
    {
        prefix = "[INFO] ";
        stream = &deb_log_stream_info;
    }
    break;

    default:
        deb_log(Deb_LogType_Error, "Invalid log message type: %d", type);

    }

    va_list args;
    va_start(args, format);

    fprintf(*stream, prefix);
    fprintf(*stream, format, args);
    fprintf(*stream, "\r\n");

    va_end(args);
}

void deb_init()
{
    deb_log_init();
}

void deb_panic(const char* message)
{
    deb_log(Deb_LogType_Error, "Panic! %s", message);
    exit(1);
}

bool deb_cmd_append_impl(Deb_Cmd* cmd, size_t args_count, ...)
{
    if (cmd == NULL)
    {
        deb_log(Deb_LogType_Error, "cmd is NULL in deb_cmd_append");
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
            deb_log(Deb_LogType_Error, "Argument is NULL in deb_cmd_append");
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
        deb_log(Deb_LogType_Error, "cmd is NULL in deb_cmd_execute");
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
        deb_log(Deb_LogType_Error, "Couldn't execute: %s", input);
        return false;
    }

    return true;
}

bool deb_dir_exists(const char* path)
{
    DWORD attrs = GetFileAttributesA(path);

    return
        GetLastError() != ERROR_FILE_NOT_FOUND &&
        attrs != INVALID_FILE_ATTRIBUTES &&
        attrs & FILE_ATTRIBUTE_DIRECTORY;
}

/* Example:
*   char* buf;
*
*   while (buf = deb_dir_list("C:\\Example"))
*       printf("%s\n", buf);
*/
char* deb_dir_list(const char* path)
{
    static WIN32_FIND_DATAA find_data;
    static HANDLE find_handle = INVALID_HANDLE_VALUE;
    static char* modified_path = NULL;

    if (path == NULL)
        goto end;

    if (find_handle == INVALID_HANDLE_VALUE)
    {
        modified_path = (char*)calloc(strlen(path) + 4, sizeof(char));
        strcpy(modified_path, path);
        strcat(modified_path, "\\*");

        find_handle = FindFirstFileA(modified_path, &find_data);

        if (find_handle == INVALID_HANDLE_VALUE)
        {
            deb_log(Deb_LogType_Error, "Invalid path: %s", path);
            free(modified_path);
            return NULL;
        }
    }

    if (FindNextFileA(find_handle, &find_data) != 0)
        return find_data.cFileName; // a filename lives until the next execution of this line

end:
    if (modified_path != NULL)
        free(modified_path);

    FindClose(find_handle);
    find_handle = INVALID_HANDLE_VALUE;

    return NULL;
}

bool deb_file_exists(const char* path)
{
    DWORD attrs = GetFileAttributesA(path);

    return
        GetLastError() != ERROR_FILE_NOT_FOUND &&
        attrs != INVALID_FILE_ATTRIBUTES &&
        (attrs & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

bool deb_file_copy(const char* src_path, const char* dest_path)
{
    if (CopyFileA(src_path, dest_path, FALSE) == TRUE)
        return true;

    deb_log(Deb_LogType_Error, "Couldn't copy a file %s to %s: %s", src_path, dest_path, deb_get_win32_error());

    return false;
}

#endif
