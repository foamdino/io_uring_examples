#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>

#define BLOCK_SZ 4096

/*
 * Returns the size of the file whose open file descriptor is passed in.
 * Properly handles regular file and block devices as well. Pretty.
 */
off_t get_file_size(int fd)
{
    struct stat st;

    if (fstat(fd, &st) < 0)
    {
        perror("fstat");
        return -1;
    }

    // block based - like a disk
    if (S_ISBLK(st.st_mode))
    {
        unsigned long long bytes;
        if (ioctl(fd, BLKGETSIZE64, &bytes) != 0)
        {
            perror("ioctl");
            return -1;
        }
    }
    else if S_ISREG(st.st_mode)
        return st.st_size;

    // something we don't support - terminal/pipe etc
    return -1;
}

/*
 * Output a string of characters of len length to stdout.
 * We use buffered output here to be efficient,
 * since we need to output character-by-character.
 */
void output_to_console(char *buf, int len)
{
    while (len--) 
    {
        fputc(*buf++, stdout);
    }
}

int read_and_print_file(char* filename)
{
    struct iovec *iovecs;
    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0) 
    {
        perror("open");
        return 1;
    }

    off_t file_sz = get_file_size(file_fd);
    if (file_sz == -1)
    {
        fprintf(stderr, "unable to get_file_size of %s", filename);
        return 1;
    }

    off_t bytes_remaining = file_sz;
    int blocks = (int) file_sz / BLOCK_SZ;
    // add remaining block
    if (file_sz % BLOCK_SZ) 
        blocks++;

    iovecs = malloc(sizeof(struct iovec) * blocks);
    int current_block = 0;

    /*
     * For the file we're reading, allocate enough blocks to be able to hold
     * the file data. Each block is described in an iovec structure, which is
     * passed to readv as part of the array of iovecs.
     */
    while (bytes_remaining)
    {
        off_t bytes_to_read = bytes_remaining;
        if (bytes_to_read > BLOCK_SZ)
            bytes_to_read = BLOCK_SZ;

        void *buf;

        if (posix_memalign(&buf, BLOCK_SZ, BLOCK_SZ))
        {
            perror("posix_memalign");
            return 1;
        }

        // setup iovecs - one per block...
        iovecs[current_block].iov_base = buf;
        iovecs[current_block].iov_len = bytes_to_read;
        current_block++;
        bytes_remaining -= bytes_to_read;
    }

    /*
     * The readv() call will block until all iovec buffers are filled with
     * file data. Once it returns, we should be able to access the file data
     * from the iovecs and print them on the console.
     */
    // use iovecs as they don't need to be contiguously allocated
    int ret = readv(file_fd, iovecs, blocks);

    if (ret < 0)
    {
        perror("readv");
        return 1;
    }

    for(int i; i < blocks; i++)
        output_to_console(iovecs[i].iov_base, iovecs[i].iov_len);

    return 0;
}

int main(int argc, char *argv[]) 
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <filename> [<filename2>]\n", argv[0]);
        return 1;
    }

    for (int i=1; i < argc; i++)
    {
        // need to flush or get no output
        fflush(stdout);
        if (read_and_print_file(argv[i]))
        {
            fprintf(stderr, "Error reading file\n");
            return 1;
        }
    }

    return 0;
}