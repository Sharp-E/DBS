#include "storage/FSFile.h"

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <algorithm>

#include "utils/zerobuf.h"

namespace taco {

static bool
get_fd_size(int fd, size_t *size) {
    struct stat st;
    if (fstat(fd, &st) != 0) {
        return false;
    }
    *size = (size_t) st.st_size;
    return true;
}

FSFile::FSFile(int fd, const std::string& path, bool o_direct, size_t size):
    m_fd(fd), m_path(path), m_o_direct(o_direct), m_size(size) {}

FSFile*
FSFile::Open(const std::string& path, bool o_trunc,
             bool o_direct, bool o_creat, mode_t mode) {
    errno = 0;
    if (path.empty()) {
        return nullptr;
    }
    int flags = O_RDWR;
    if (o_trunc) flags |= O_TRUNC;
    if (o_direct) flags |= O_DIRECT;
    if (o_creat) flags |= O_CREAT;
    int fd = open(path.c_str(), flags, mode);
    if (fd < 0) {
        return nullptr;
    }
    size_t size;
    if (!get_fd_size(fd, &size)) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return nullptr;
    }
    return new FSFile(fd, path, o_direct, size);
}

FSFile::~FSFile() {
    Close();
}

bool
FSFile::Reopen() {
    errno = 0;
    if (IsOpen()) {
        Close();
    }
    int flags = O_RDWR;
    if (m_o_direct) flags |= O_DIRECT;
    int fd = open(m_path.c_str(), flags);
    if (fd < 0) {
        return false;
    }
    size_t size;
    if (!get_fd_size(fd, &size)) {
        int saved_errno = errno;
        close(fd);
        errno = saved_errno;
        return false;
    }
    m_fd = fd;
    m_size = size;
    return true;
}

void
FSFile::Close() {
    if (m_fd < 0) {
        return;
    }
    if (close(m_fd) != 0) {
        LOG(kWarning, "failed to close file %s: %s",
            m_path.c_str(), strerror(errno));
    }
    m_fd = -1;
}

bool
FSFile::IsOpen() const {
    return m_fd >= 0;
}

void
FSFile::Delete() const {
    if (unlink(m_path.c_str()) != 0) {
        LOG(kWarning, "failed to delete file %s: %s",
            m_path.c_str(), strerror(errno));
    }
}

void
FSFile::Read(void *buf, size_t count, off_t offset) {
    if (offset < 0 || (size_t) offset > m_size ||
        count > m_size - (size_t) offset) {
        LOG(kFatal, "read out of range on file %s", m_path.c_str());
    }
    size_t done = 0;
    while (done < count) {
        ssize_t n = pread(m_fd, (char *) buf + done, count - done,
                          offset + (off_t) done);
        if (n < 0) {
            if (errno == EINTR) continue;
            LOG(kFatal, "pread failed: %s", strerror(errno));
        }
        if (n == 0) {
            LOG(kFatal, "partial read on file %s", m_path.c_str());
        }
        done += (size_t) n;
    }
}

void
FSFile::Write(const void *buf, size_t count, off_t offset) {
    if (offset < 0 || (size_t) offset > m_size ||
        count > m_size - (size_t) offset) {
        LOG(kFatal, "write out of range on file %s", m_path.c_str());
    }
    size_t done = 0;
    while (done < count) {
        ssize_t n = pwrite(m_fd, (const char *) buf + done, count - done,
                           offset + (off_t) done);
        if (n < 0) {
            if (errno == EINTR) continue;
            LOG(kFatal, "pwrite failed: %s", strerror(errno));
        }
        if (n == 0) {
            LOG(kFatal, "partial write on file %s", m_path.c_str());
        }
        done += (size_t) n;
    }
}

void
FSFile::Allocate(size_t count) {
    if (count == 0) {
        return;
    }
    off_t offset = (off_t) m_size;
    if (fallocate_zerofill_fast(m_fd, offset, (off_t) count)) {
        m_size += count;
        return;
    }
    if (errno != 0 && errno != EOPNOTSUPP) {
        LOG(kFatal, "fallocate failed: %s", strerror(errno));
    }
    static const size_t CHUNK = 1 << 20;
    alignas(4096) static char zeros[CHUNK] = {};
    size_t done = 0;
    while (done < count) {
        size_t len = std::min(CHUNK, count - done);
        ssize_t n = pwrite(m_fd, zeros, len, offset + (off_t) done);
        if (n < 0) {
            if (errno == EINTR) continue;
            LOG(kFatal, "failed to extend file: %s", strerror(errno));
        }
        if (n == 0) {
            LOG(kFatal, "failed to extend file %s", m_path.c_str());
        }
        done += (size_t) n;
    }
    m_size += count;
}

size_t
FSFile::Size() const noexcept {
    return m_size;
}

void
FSFile::Flush() {
    if (fsync(m_fd) != 0) {
        LOG(kFatal, "fsync failed: %s", strerror(errno));
    }
}

}
