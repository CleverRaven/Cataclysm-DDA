#pragma once

#ifdef __NetBSD__
#  include "pal_bsd_aligned.h"

#  include <fcntl.h>
#  include <sys/syscall.h>

/**
 * We skip the pthread cancellation checkpoints by reaching directly
 * the following syscalls so we avoid the possible pthread
 * allocation initialization timing issues.
 * @{
 */
extern "C" ssize_t _sys_writev(int fd, const struct iovec* iov, int iovcnt);
extern "C" int _sys_fsync(int fd);

/// @}

namespace snmalloc
{
  /**
   * NetBSD-specific platform abstraction layer.
   *
   * This adds NetBSD-specific aligned allocation to the generic BSD
   * implementation.
   */
  class PALNetBSD : public PALBSD_Aligned<PALNetBSD, _sys_writev, _sys_fsync>
  {
  public:
    /**
     * Bitmap of PalFeatures flags indicating the optional features that this
     * PAL supports.
     *
     * The NetBSD PAL does not currently add any features beyond those of a
     * generic BSD with support for arbitrary alignment from `mmap`.  This
     * field is declared explicitly to remind anyone modifying this class to
     * add new features that they should add any required feature flags.
     *
     * As NetBSD does not have the getentropy call, get_entropy64 will
     * currently fallback to C++ libraries std::random_device.
     */
    static constexpr uint64_t pal_features =
      PALBSD_Aligned::pal_features | Entropy;

    /**
     * Temporary solution for NetBSD < 10
     * random_device seems unimplemented in clang for this platform
     * otherwise using getrandom
     */
    static uint64_t get_entropy64()
    {
#  if defined(SYS_getrandom)
      uint64_t result;
      if (getrandom(&result, sizeof(result), 0) != sizeof(result))
        error("Failed to get system randomness");
      return result;
#  else
      return PALPOSIX::dev_urandom();
#  endif
    }
  };
} // namespace snmalloc
#endif
