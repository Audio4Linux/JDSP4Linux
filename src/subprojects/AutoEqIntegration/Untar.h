#ifndef UNTAR_H
#define UNTAR_H

#include <QDebug>
#include <QString>
#include <QDir>

#include <archive.h>
#include <archive_entry.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class Untar
{
public:
    static int extract(const QString& qFilename, const QDir& outputPath, QString& returnState)
    {
        const char* filename = qFilename.toUtf8().constData();

        struct archive *a;
        struct archive *ext;
        struct archive_entry *entry;
        int flags;
        int r;

        /* Select which attributes we want to restore. */
        flags = ARCHIVE_EXTRACT_TIME;

        a = archive_read_new();
        archive_read_support_format_all(a);
        archive_read_support_filter_all(a);
        ext = archive_write_disk_new();
        archive_write_disk_set_options(ext, flags);
        archive_write_disk_set_standard_lookup(ext);
        if ((r = archive_read_open_filename(a, filename, 10240)))
        {
            returnState = archive_error_string(a);
            qWarning() << returnState << "\n";
            return (1);
        }
        for (;;) {
            r = archive_read_next_header(a, &entry);
            if (r == ARCHIVE_EOF)
                break;
            if (r < ARCHIVE_OK)
                qWarning() << archive_error_string(a);
            if (r < ARCHIVE_WARN)
            {
                returnState = archive_error_string(a);
                return 1;
            }

            const char* currentFile = archive_entry_pathname(entry);
            const QString fullOutputPath = outputPath.path() + QDir::separator() + currentFile;
            archive_entry_set_pathname_utf8(entry, fullOutputPath.toUtf8().constData());

            r = archive_write_header(ext, entry);
            if (r < ARCHIVE_OK)
                qWarning() << archive_error_string(ext);
            else if (archive_entry_size(entry) > 0) {
                r = copy_data(a, ext);
                if (r < ARCHIVE_OK)
                    qWarning() << archive_error_string(ext);
                if (r < ARCHIVE_WARN)
                {
                    returnState = archive_error_string(ext);
                    return 1;
                }
            }
            r = archive_write_finish_entry(ext);
            if (r < ARCHIVE_OK)
                qWarning() << archive_error_string(ext);
            if (r < ARCHIVE_WARN)
            {
                returnState = archive_error_string(ext);
                return 1;
            }
        }
        archive_read_close(a);
        archive_read_free(a);
        archive_write_close(ext);
        archive_write_free(ext);
        return 0;
    }

private:
    static int
    copy_data(struct archive *ar, struct archive *aw)
    {
        int r;
        const void *buff;
        size_t size;
        int64_t offset;

        for (;;) {
            r = archive_read_data_block(ar, &buff, &size, &offset);
            if (r == ARCHIVE_EOF)
                return (ARCHIVE_OK);
            if (r < ARCHIVE_OK) {
                qWarning() << "copyData(): " << archive_error_string(ar) << "\n";
                return (r);
            }
            r = archive_write_data_block(aw, buff, size, offset);
            if (r < ARCHIVE_OK) {
                qWarning() << "copyData(): " << archive_error_string(ar) << "\n";
                return (r);
            }
        }
    }

};

#endif // UNTAR_H
