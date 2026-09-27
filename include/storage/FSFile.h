#ifndef STORAGE_FSFILE_H
#define STORAGE_FSFILE_H

#include "tdb.h"

#include <string>

namespace taco {

class FSFile {
public:
    static FSFile *Open(const std::string& path, bool o_trunc,
                        bool o_direct, bool o_creat,
                        mode_t mode = 0600);

    ~FSFile();

    bool Reopen();

    void Close();

    bool IsOpen() const;

    void Delete() const;

    void Read(void *buf, size_t count, off_t offset);

    void Write(const void *buf, size_t count, off_t offset);

    void Allocate(size_t count);

    size_t Size() const noexcept;

    void Flush();

private:
    FSFile(int fd, const std::string& path, bool o_direct, size_t size);

    int m_fd;
    std::string m_path;
    bool m_o_direct;
    size_t m_size;
};

bool fallocate_zerofill_fast(int fd, off_t offset, off_t len);

}

#endif
