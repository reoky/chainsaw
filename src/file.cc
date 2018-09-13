// The definition of a file
#include "file.h"

// Compiler-provided headers go first.
#include <cassert>        // assert
#include <cstring>        // memset
#include <iomanip>        // std::quoted
#include <iostream>       // std::cerr
#include <map>            // std::map
#include <stdexcept>      // std::runtime_error, nested stuff
#include <sstream>        // std::ostringstream
#include <system_error>   // std::system_category
#include <tuple>          // std::tie
#include <vector>         // std::vector

// Operating system headers go second.

// The operating system calls this structure 'stat', but that name looks
// like a value, so we'll rename it to use our types-end-in-t convention.
using stat_t = struct stat;

// The default-constructed state is as a close file.  Our file descriptor is
// the illegal one (-1).
file_t::file_t() noexcept
    : fd(illegal_fd) {}

// Move-construct, leaving the donor closed.
file_t::file_t(file_t &&donor) noexcept  {
    fd = donor.fd;
    donor.fd = illegal_fd;
    // FYI: If you want to do this same thing in a fancier, more modern C++
    // kinda way, do this:
    //    fd = std::exchange(donor.fd, illegal_fd);
  }

// Close as we go.
file_t::~file_t() {
  // Loop if we have an open file descriptor.
  while (fd >= 0) {
    // Attempt to close the file descriptor.  If it closes without any
    // difficulty, great, we're done.  (This is by far the most common
    // thing that will happen.)
    if (close(fd) == 0) {
      break;
    }
    // What went wrong?
    switch (errno) {
      // The file descriptor is already closed (somehow) so we can just
      // call it good.
      case EBADF: {
        fd = illegal_fd;
        break;
      }
      // Our call was interrupted by a system signal.  We can just try
      // again.
      case EINTR: {
        break;
      }
      // Something more sinister has gone wrong, so we better shut the
      // whole process down hard.
      default: {
        abort();
      }
    }  // switch
  }  // while
}

// Move-assign, leaving the donor closed.
file_t &file_t::operator=(file_t &&donor) noexcept {
  // It is possible for someone to attempt to move-assign us to ourself, in
  // which case we can just do nothing.  (But that dude's weird.)
  if (this != &donor) {
    // Explicitly destroy this file in-place without giving up our memory.
    // The destructor will close our file descriptor if we have one open.
    this->~file_t();
    // Explicitly re-construct a new file object at the same memory address
    // as the old one (so no one outside can tell we've changed) and use
    // the move-constructor to do our work for us.
    new (this) file_t(std::move(donor));
  }
  // Return the chaining reference required of operator=.
  return *this;
}

// The size of the file as an unsigned 64-bit integer.  The operating system
// actually uses a type called off_t to store the size, and off_t is a signed
// integer appropriate to the size of the host file system.  But we'll
// simplify our lives if we just use uint64_t here.
std::pair<uint64_t, mode_t> file_t::get_size_and_mode() const {
  assert(fd >= 0);
  stat_t stat;
  if (fstat(fd, &stat) < 0) {
    throw std::system_error { errno, std::system_category() };
  }
  return { static_cast<uint64_t>(stat.st_size), stat.st_mode };
}

// Read at most max_size bytes from the file and store them in buffer.
// Return the actual number of bytes read.
size_t file_t::read_at_most(char *buffer, size_t max_size) {
  assert(fd >= 0);
  // The operating system call read() reads data from a file descriptor for
  // us and returns the number of bytes it read.  If there is an error, it
  // returns -1.  This means the return type, ssize_t, must be signed.
  // But we want to return a normal size_t (which is unsigned) so, after
  // checking for an error, we cast the result.
  ssize_t result = read(fd, buffer, max_size);
  if (result < 0) {
    throw std::system_error { errno, std::system_category() };
  }
  return static_cast<size_t>(result);
}

// Read exactly size bytes from the file to the buffer.
void file_t::read_exactly(char *buffer, size_t size) {
  assert(fd >= 0);
  while (size) {
    ssize_t result = read(fd, buffer, size);
    if (result < 0) {
      throw std::system_error { errno, std::system_category() };
    }
    if (result == 0) {
      throw std::runtime_error { "Unexpected end of file." };
    }
    auto read_size = static_cast<size_t>(result);
    buffer += read_size;
    size   -= read_size;
  }
}

// Seek to a new position within the file.  The offset is relative to either
// the start of the file (whence=SEEK_SET), our current position within the
// file (whence=SEEK_CUR), or from the end of the file (whence=SEEK_END).
// Returns the new position in the file as the number of bytes from the
// start.
uint64_t file_t::seek(int64_t offset, int whence) {
  assert(fd >= 0);
  auto result = lseek64(fd, offset, whence);
  if (result < 0) {
    throw std::system_error { errno, std::system_category() };
  }
  return static_cast<uint64_t>(result);
}

// Write exactly size bytes from buffer to the file.
void file_t::write_exactly(const char *buffer, size_t size) {
  assert(fd >= 0);
  // Loop while we still have bytes to write.
  while (size) {
    // The operating system call write() is similar to read() in that it
    // returns the number of bytes actually written.  If there is an error,
    // it returns -1.  As before, we check for an error, then cast away
    // the sign.
    ssize_t result = write(fd, buffer, size);
    if (result < 0) {
      throw std::system_error { errno, std::system_category() };
    }
    auto write_size = static_cast<size_t>(result);
    // Bump the buffer pointer ahead and decrement the number of bytes
    // we have left to write.
    buffer += write_size;
    size   -= write_size;
  }
}

// Return a newly constructed file object with the file open for reading.
// If the file doesn't exist, this throws.
file_t file_t::open_ro(const std::string &path) {
  // Work inside of a try-block so we can nest a descriptive error message if
  // anything goes wrong.
  file_t result;
  try {
    // Have the operating sytem open the file for us and return us a handle,
    // called a file descriptor, which we can use to refer to the open file.
    // If this call fails, it returns a negative number (valid file
    // descriptors are always non-negative), and we consult errno to find
    // out what went wrong and throw an appropriate exception.
    result.fd = open(path.c_str(), O_RDONLY);
    if (result.fd < 0) {
      throw std::system_error { errno, std::system_category() };
    }
  } catch (...) {
    // Catch whatever went wrong and nest in inside of a message indicating
    // what we were trying to do.  We use a string-builder approach here
    // (using the std::ostringstream class to build a string in memory),
    // then use the built string to form an exception.  The exception we
    // caught will be nested inside this new exception, so the resulting
    // message will unnest as something like:
    //    could not open "foo"; no such file or directory
    std::ostringstream msg;
    msg << "Could not open " << std::quoted(path) << " for reading.";
    std::throw_with_nested(std::runtime_error { msg.str() });
  }
  return result;
}

// Return a newly constructed file object with the file open for reading and
// writing.  If the file doesn't exist, it will be created.  If it does
// exist, it will be truncated.
file_t file_t::open_rw(const std::string &path, mode_t mode) {
  // This works pretty much the same as open_ro(), except that the flags
  // we pass to the operating system open() function are different and we
  // print a slightly different error message if anything goes wrong.
  file_t result;
  try {
    result.fd = open(path.c_str(), O_RDWR | O_CREAT | O_TRUNC, mode);
    if (result.fd < 0) {
      throw std::system_error { errno, std::system_category() };
    }
  } catch (...) {
    std::ostringstream msg;
    msg << "Could not open " << std::quoted(path) << " for writing.";
    std::throw_with_nested(std::runtime_error { msg.str() });
  }
  return result;
}