#include <sched.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

namespace ns_utils {

inline bool enter_init_mount_ns() {
    int fd = ::open("/proc/1/ns/mnt", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        perror("open /proc/1/ns/mnt failed");
        return false;
    }

    const int ret = ::setns(fd, CLONE_NEWNS);
    const int saved_errno = errno;
    ::close(fd);

    if (ret < 0) {
        errno = saved_errno;
        // perror("setns CLONE_NEWNS failed");
        return false;
    }
    return true;
}
} // namespace ns_utils