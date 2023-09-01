
// (C) Shin'ichi Ichikawa. Released under the MIT license.

#define WINVER 0x0A00       // Windows 10
#define _WIN32_WINNT 0x0A00 // Windows 10
#define MICROSOFT_WINDOWS_WINBASE_H_DEFINE_INTERLOCKED_CPLUSPLUS_OVERLOADS 0
#include <sdkddkver.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <malloc.h>
#include <errno.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

#define X86_CODE_DEFAULT_FNAME "x86"
#define X86_CODE_DEFAULT_EXT "bin"
#define X86_CODE_MAX_FILE_BYTES 8192u
#define X86_CODE_ALLOCA_SIZE 256
#define X86_OPCODE_NOP 0x90

#define X86_MESSAGE_BUFFER_SIZE 1024

static bool exec_x86_code(int* return_value);
static bool open_read_file(char* path, FILE** file_stream, uint32_t* x86_code_buffer_size);
static bool read_file(FILE* file_stream, uint8_t* buffer, size_t buffer_size);
static bool close_file(FILE** file_stream);
static bool get_drive_dir(char* drive, char* dir);
static bool make_path(
    char* drive, char* dir, char* fname, char* ext,
    char* path, size_t path_size
);
static void print_crt_error(uint8_t* func, size_t line_num);
static void get_last_error(uint8_t* func, size_t line_num);

typedef int (*x86_func)(void);

int main(void)
{
    errno_t err = _set_errno(0);
    SetLastError(NO_ERROR);

    BOOL Wow64Process = FALSE;

    if ( ! IsWow64Process(GetCurrentProcess(), &Wow64Process)){

        get_last_error(__func__, __LINE__);

        return EXIT_FAILURE;
    }

    if (Wow64Process){

        fprintf(stderr, "ERROR: Running in 32-bit mode.\n");

        return EXIT_FAILURE;
    }

    int return_value = 0;

    // NOTE: It is assumed to run in 64-bit mode on x64 systems.
    if ( ! exec_x86_code(&return_value)){

        fprintf(stderr, "Execution of exec_x86_code() failed.\n");

        return EXIT_FAILURE;
    }

    fprintf(stderr, "press enter key:\n");
    (void)getchar();

    return return_value;
}

static bool exec_x86_code(int* return_value)
{
    // NOTE: Running in 64-bit mode.
    bool status = false;
    uint8_t* x86_code_buffer = NULL;
    uint32_t x86_code_buffer_size = 0;

    FILE* x86_code_file_stream = NULL;

    char drive[_MAX_DRIVE] = { 0 };
    char dir[_MAX_DIR] = { 0 };

    if ( ! get_drive_dir(drive, dir)){

        fprintf(stderr, "TRACE get_drive_dir() : %s(%d)\n", __func__, __LINE__);

        goto fail;
    }

    strcat_s(dir, sizeof(dir), "..\\tools\\");

    char load_x86_bin_file_path[_MAX_PATH] = { 0 };

    if ( ! make_path(
        drive, dir, X86_CODE_DEFAULT_FNAME, X86_CODE_DEFAULT_EXT,
        load_x86_bin_file_path, sizeof(load_x86_bin_file_path))){

        fprintf(stderr, "TRACE make_path() : %s(%d)\n", __func__, __LINE__);

        goto fail;
    }

    if ( ! open_read_file(load_x86_bin_file_path, &x86_code_file_stream, &x86_code_buffer_size)){

        fprintf(stderr, "TRACE open_read_file() : %s(%d)\n", __func__, __LINE__);

        goto fail;
    }

    uint8_t* tmp_x86_code_buffer = (uint8_t*)VirtualAlloc(
        NULL, x86_code_buffer_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE
    );

    if (NULL == tmp_x86_code_buffer){

        get_last_error(__func__, __LINE__);

        goto fail;
    }

    x86_code_buffer = tmp_x86_code_buffer;

    memset(x86_code_buffer, X86_OPCODE_NOP, x86_code_buffer_size);

    if ( ! read_file(x86_code_file_stream, x86_code_buffer, x86_code_buffer_size)){

        fprintf(stderr, "TRACE read_file() : %s(%d)\n", __func__, __LINE__);

        goto fail;
    }

    if ( ! close_file(&x86_code_file_stream)){

        fprintf(stderr, "TRACE close_file() : %s(%d)\n", __func__, __LINE__);

        goto fail;
    }

    DWORD old_protect = 0;

    if ( ! VirtualProtect(x86_code_buffer, x86_code_buffer_size, PAGE_EXECUTE_READ, &old_protect)){

        get_last_error(__func__, __LINE__);

        goto fail;
    }

    int value = 0;

    __try{

        uint8_t* p = (uint8_t*)_alloca(X86_CODE_ALLOCA_SIZE);

        x86_func func = (x86_func)x86_code_buffer;

        value = func();
    }__except (STATUS_STACK_OVERFLOW == GetExceptionCode()){

        fprintf(stderr, "ERROR: stack overflows.\n");

        int errcode = _resetstkoflw();

        if (errcode == 0){

            fprintf(stderr, "ERROR: Could not reset the stack.\n");

            exit(EXIT_FAILURE);
        }

        goto fail;
    }

    printf("x86_func() = %d\n", value);

    if (return_value){

        *return_value = value;
    }

    if ( ! VirtualProtect(x86_code_buffer, x86_code_buffer_size, PAGE_READWRITE, &old_protect)){

        get_last_error(__func__, __LINE__);

        goto fail;
    }

    status = true;

fail:
    ;

    if (x86_code_buffer){

        memset(x86_code_buffer, 0, x86_code_buffer_size);

        if ( ! VirtualFree(x86_code_buffer, 0, MEM_RELEASE)){

            get_last_error(__func__, __LINE__);
        }

        x86_code_buffer = NULL;
        x86_code_buffer_size = 0;
    }

    return status;
}

static bool open_read_file(char* path, FILE** file_stream, uint32_t* x86_code_buffer_size)
{
    int fd = 0;

    errno_t _sopen_s_error = _sopen_s(&fd, path, _O_RDONLY | _O_BINARY, _SH_DENYWR, 0);

    if (_sopen_s_error){

        print_crt_error(__func__, __LINE__);

        return false;
    }
  
    FILE* stream = _fdopen(fd, "rb");

    if (NULL == stream){

        print_crt_error(__func__, __LINE__);

        return false;
    }

    *file_stream = stream;

    struct stat stbuf;

    int fstat_error = fstat(fd, &stbuf);

    if (-1 == fstat_error){

        print_crt_error(__func__, __LINE__);

        clearerr(stream);

        return false;
    }

    if (X86_CODE_MAX_FILE_BYTES < stbuf.st_size){

        fprintf(stderr, "ERROR: X86_MAX_FILE_BYTES(%du) < stbuf.st_size(%du) : %s(%d)\n",
            X86_CODE_MAX_FILE_BYTES, stbuf.st_size, __func__, __LINE__
        );

        return false;
    }

    *x86_code_buffer_size = stbuf.st_size;

    return true;
}

static bool read_file(FILE* file_stream, uint8_t* buffer, size_t buffer_size)
{
    size_t fread_bytes = fread(
        buffer, sizeof(uint8_t), buffer_size, file_stream
    );

    if (buffer_size > fread_bytes){

        int ferror_error = ferror(file_stream);

        if (ferror_error){

            print_crt_error(__func__, __LINE__);

            clearerr(file_stream);

            return false;
        }

        int feof_error = feof(file_stream);

        if (feof_error){

            fprintf(stderr, "feof_error : %s(%d)\n", __func__, __LINE__);

            return false;
        }
    }

    return true;
}

static bool close_file(FILE** file_stream)
{
    if (NULL == file_stream){

        return true;
    }

    if (NULL == *file_stream){

        return true;
    }

    int fclose_error = fclose(*file_stream);

    if (EOF == fclose_error){

        print_crt_error(__func__, __LINE__);

        clearerr(*file_stream);

        *file_stream = NULL;

        return false;
    }

    *file_stream = NULL;

    return true;
}

static bool get_drive_dir(char* drive, char* dir)
{
    char base_dir[_MAX_PATH] = { 0 };

    HMODULE handle = GetModuleHandleA(NULL);

    if (0 == handle){

        get_last_error(__func__, __LINE__);

        return false;
    }

    DWORD module_status = GetModuleFileNameA(handle, base_dir, sizeof(base_dir));

    if (0 == module_status){

        get_last_error(__func__, __LINE__);

        return false;
    }

    errno_t err = _splitpath_s(base_dir, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0);

    if (err){

        print_crt_error(__func__, __LINE__);

        return false;
    }

    return true;
}

static bool make_path(
    char* drive, char* dir, char* fname, char* ext,
    char* path, size_t path_size)
{
    char tmp_fname[_MAX_FNAME] = { 0 };

    sprintf_s(tmp_fname, sizeof(tmp_fname), "%s", fname);

    errno_t err = _makepath_s(path, path_size, drive, dir, tmp_fname, ext);

    if (err){

        print_crt_error(__func__, __LINE__);

        return false;
    }

    return true;
}

static void print_crt_error(uint8_t* func, size_t line_num)
{
    if (errno){

        char msg_buffer[X86_MESSAGE_BUFFER_SIZE] = { 0 };

        errno_t err = strerror_s(msg_buffer, sizeof(msg_buffer), errno);

        fprintf(stderr, "%s(%zu) : %s\n", func, line_num, msg_buffer);

        err = _set_errno(0);
    }
}

static void get_last_error(uint8_t* func, size_t line_num)
{
    LPVOID msg_buffer = NULL;

    FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPSTR)&msg_buffer,
        0,
        NULL
    );

    fprintf(stderr, "%s(%zu) : %s\n", func, line_num, (char*)msg_buffer);

    LocalFree(msg_buffer);
    msg_buffer = NULL;

    SetLastError(NO_ERROR);

    errno_t err = _set_errno(0);
}

