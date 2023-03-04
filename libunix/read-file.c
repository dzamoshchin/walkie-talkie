#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include "libunix.h"

// allocate buffer, read entire file into it, return it.   
// buffer is zero padded to a multiple of 4.
//
//  - <size> = exact nbytes of file.
//  - for allocation: round up allocated size to 4-byte multiple, pad
//    buffer with 0s. 
//
// fatal error: open/read of <name> fails.
//   - make sure to check all system calls for errors.
//   - make sure to close the file descriptor (this will
//     matter for later labs).
// 
void *read_file(unsigned *size, const char *name) {
    // How: 
    //    - use stat to get the size of the file.
    //    - round up to a multiple of 4.
    //    - allocate a buffer
    //    - zero pads to a multiple of 4.
    //    - read entire file into buffer.  
    //    - make sure any padding bytes have zeros.
    //    - return it.   

    int fd = open(name, O_RDONLY);
    if (fd < 0) {
        sys_die(open, "file descriptor for file %s is negative (can't read).\n", name);
    }

    struct stat s;
    if (fstat(fd, &s) < 0) {
        sys_die(fstat, "can't get size of file %s.\n", name);
    }

    void* buf = calloc(1, s.st_size + 4);
    assert(buf);

    int n = read(fd, buf, s.st_size);
    if (n != s.st_size) {
        panic("%s: expected to read %lu bytes but read %d bytes.\n", name, (unsigned long) s.st_size, n);
    }

    close(fd);
    *size = n;
    return buf;
}
