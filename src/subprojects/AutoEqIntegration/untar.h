#ifndef UNTAR_H
#define UNTAR_H

/* Based on https://github.com/libarchive/libarchive/blob/master/contrib/untar.c by Tim Kientzle, March 2009
 * This file is in the public domain. Use it as you see fit. */

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <sys/stat.h>  /* For mkdir() */

#include <filesystem>

/* Parse an octal number, ignoring leading and trailing nonsense. */
static int
parseoct(const char *p, size_t n)
{
    int i = 0;

    while ((*p < '0' || *p > '7') && n > 0) {
        ++p;
        --n;
    }
    while (*p >= '0' && *p <= '7' && n > 0) {
        i *= 8;
        i += *p - '0';
        ++p;
        --n;
    }
    return (i);
}

/* Returns true if this is 512 zero bytes. */
static int
is_end_of_archive(const char *p)
{
    int n;
    for (n = 511; n >= 0; --n)
        if (p[n] != '\0')
            return (0);
    return (1);
}

/* Create a directory, including parent directories as necessary. */
static void
create_dir(const char *pathname, int mode, const char *path)
{
    std::error_code ec;
    std::filesystem::path full_path = std::filesystem::path(path) / std::filesystem::path(pathname);
    std::filesystem::create_directories(full_path, ec);

    if (ec) fprintf(stderr, "Could not create directory %s\n", pathname);
}

/* Create a file, including parent directory as necessary. */
static FILE *
create_file(char *pathname_in, int mode, const char *path)
{

  const auto longlink_path = std::filesystem::path("@LongLink");
  std::filesystem::path full_path = std::filesystem::path(path) / std::filesystem::path(pathname_in);
  std::string pathname(full_path);

  if (
    // if this pathname_in isn't @LongLink...
    longlink_path != std::filesystem::path(pathname)
    // ... and there exists an @LongLink file
    && std::filesystem::exists(longlink_path)
  ) {

    std::cout << " @LongLink detected" << std::endl;
    std::cout << " Renaming " << pathname << " to @LongLink contents";
    std::cout << std::endl;

    // then set the pathname to the contents of @LongLink...
    std::ifstream longlink_stream(longlink_path);
    pathname = std::string(
      std::istreambuf_iterator<char>(longlink_stream),
      std::istreambuf_iterator<char>()
    );
    // ... and delete the @LongLink file
    std::filesystem::remove(longlink_path);
  }

    FILE *f;
    f = fopen(pathname.c_str(), "wb+");
    if (f == NULL) {
        /* Try creating parent dir and then creating file. */
        create_dir(
      std::filesystem::path(pathname).parent_path().c_str(),
      0755, ""
    );
        f = fopen(pathname.c_str(), "wb+");
  }

    return (f);
}

/* Verify the tar checksum. */
static int
verify_checksum(const char *p)
{
    int n, u = 0;
    for (n = 0; n < 512; ++n) {
        if (n < 148 || n > 155)
            /* Standard tar checksum adds unsigned bytes. */
            u += ((unsigned char *)p)[n];
        else
            u += 0x20;

    }
    return (u == parseoct(p + 148, 8));
}

#include <QDebug>

/* Extract a tar archive. */
static void
untar(FILE *a, const char *path, bool* bad_checksum, bool* has_short_read)
{
    char buff[512];
    FILE *f = NULL;
    size_t bytes_read;
    int filesize;

    *bad_checksum = false;
    *has_short_read = false;

    for (;;) {
        bytes_read = fread(buff, 1, 512, a);
        if (bytes_read < 512) {
            fprintf(stderr,
                "Short read: expected 512, got %d\n",
                (int)bytes_read);
            *has_short_read = true;
            return;
        }
        if (is_end_of_archive(buff)) {
            printf("End of archive\n");
            return;
        }
        if (!verify_checksum(buff)) {
            fprintf(stderr, "Checksum failure\n");
            *bad_checksum = true;
            return;
        }
        filesize = parseoct(buff + 124, 12);
        switch (buff[156]) {
        case '1':
            printf(" Ignoring hardlink %s\n", buff);
            break;
        case '2':
            printf(" Ignoring symlink %s\n", buff);
            break;
        case '3':
            printf(" Ignoring character device %s\n", buff);
                break;
        case '4':
            printf(" Ignoring block device %s\n", buff);
            break;
        case '5':
            //printf(" Extracting dir %s\n", buff);
            create_dir(buff, parseoct(buff + 100, 8), path);
            filesize = 0;
            break;
        case '6':
            printf(" Ignoring FIFO %s\n", buff);
            break;
        default:
            //printf(" Extracting file %s\n", buff);
            f = create_file(buff, parseoct(buff + 100, 8), path);
            break;
        }
        while (filesize > 0) {
            bytes_read = fread(buff, 1, 512, a);
            if (bytes_read < 512) {
                fprintf(stderr,
                    "Short read: Expected 512, got %d\n",
                    (int)bytes_read);
                *has_short_read = true;
                return;
            }
            if (filesize < 512)
                bytes_read = filesize;
            if (f != NULL) {
                if (fwrite(buff, 1, bytes_read, f)
                    != bytes_read)
                {
                    fprintf(stderr, "Failed write\n");
                    fclose(f);
                    f = NULL;
                }
            }
            filesize -= bytes_read;
        }
        if (f != NULL) {
            fclose(f);
            f = NULL;
        }
    }
}
#endif // UNTAR_H
