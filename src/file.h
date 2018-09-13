#pragma once
#include <cstdint>        // uint64_t
#include <string>         // std::string
#include <utility>        // std::min

#include <sys/stat.h>
#include <fcntl.h>        // open()
#include <unistd.h>       // close()

// Provides object-oriented handling of an operating system file handle.  This
// class employs the RAII technique (Resource Allocation Is Initialization) to
// manage the file descriptor, preventing leaks.
class file_t final {
public:

  // The operating system calls this structure 'stat', but that name looks
  // like a value, so we'll rename it to use our types-end-in-t convention.
  using stat_t = struct stat;

  // The default-constructed state is as a close file.  Our file descriptor is
  // the illegal one (-1).
  file_t() noexcept;

  // Move-construct, leaving the donor closed.
  file_t(file_t &&donor) noexcept;

  // Copying is not allowed.
  file_t(const file_t &) = delete;

  // Close as we go.
  ~file_t();

  // Move-assign, leaving the donor closed.
  file_t &operator=(file_t &&donor) noexcept;

  // Copying is not allowed.
  file_t &operator=(const file_t &) = delete;

  // The size of the file as an unsigned 64-bit integer.  The operating system
  // actually uses a type called off_t to store the size, and off_t is a signed
  // integer appropriate to the size of the host file system.  But we'll
  // simplify our lives if we just use uint64_t here.
  std::pair<uint64_t, mode_t> get_size_and_mode() const;

  // Read at most max_size bytes from the file and store them in buffer.
  // Return the actual number of bytes read.
  size_t read_at_most(char *buffer, size_t max_size);

  // Read exactly size bytes from the file to the buffer.
  void read_exactly(char *buffer, size_t size);

  // Seek to a new position within the file.  The offset is relative to either
  // the start of the file (whence=SEEK_SET), our current position within the
  // file (whence=SEEK_CUR), or from the end of the file (whence=SEEK_END).
  // Returns the new position in the file as the number of bytes from the
  // start.
  uint64_t seek(int64_t offset, int whence);
  // Write exactly size bytes from buffer to the file.
  void write_exactly(const char *buffer, size_t size);

  // Return a newly constructed file object with the file open for reading.
  // If the file doesn't exist, this throws.
  static file_t open_ro(const std::string &path);
  // Return a newly constructed file object with the file open for reading and
  // writing.  If the file doesn't exist, it will be created.  If it does
  // exist, it will be truncated.
  static file_t open_rw(const std::string &path, mode_t mode = 0777);
private:

  // All negative integers are illegal file descriptors, but we standardize
  // on this one for our purposes.
  // constexpr is available to the compiler during compilation
  static constexpr int illegal_fd = -1;

  // The descriptor of the file we currently have open, or illegal_fd if we
  // are currently closed.
  int fd;

};  // file_t