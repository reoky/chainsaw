// Compiler-provided headers go first.
#include <cassert>        // assert
#include <cstdint>        // uint64_t
#include <cstring>        // memset
#include <iomanip>        // std::quoted
#include <iostream>       // std::cerr
#include <stdexcept>      // std::runtime_error, nested stuff
#include <sstream>        // std::ostringstream
#include <string>         // std::string
#include <system_error>   // std::system_category
#include <tuple>          // std::tie
#include <utility>        // std::min
#include <vector>         // std::vector

// Operating system headers go second.
#include <fcntl.h>        // open()
#include <unistd.h>       // close()
#include <stdlib.h>       // atoi
#include <sys/stat.h>     // fstat()

// Chainsaw headers
#include "crc.h"
#include "file.h"
#include "help.h"
#include "join.h"
#include "shard_hdr.h"
#include "split.h"

// A class representing the application itself.  We never make more than one
// of these, but it's a convenient way to express the startup-run-teardown
// mechanism of app-running.
class app_t final {
public:

  // The constructor parses the command-line arguments passed to us by the
  // operating system.  If we encounter any problems here, we throw, which
  // will cause the process to shut down with a nice error message.
  app_t(int argc, char *argv[]) {

    // Check for any args at all
    assert(argc >= 1);

    // Nab argv[0], which is the name of the app itself.
    app_name = *argv++;
    --argc;

    // Nab all the file names.
    while (argc) {
      app_params.emplace_back(*argv++);
      --argc;
    }  // while

    // A zero sized shard means split the file into 8 shards
    max_shard_size = 0;
    make_directory = false;
    shard_prefix = "shard";

    // Arg parse
    try {
      for (int i = 0; i < app_params.size(); i++) {

        // Check for the size param
        if (app_params[i] == "-s") {
          max_shard_size = atoi(app_params[++i].c_str());
          if (max_shard_size < 1) {
            throw std::runtime_error { "Shards should be at least 1MB in size." };
          }
          max_shard_size = max_shard_size * 1024 * 1024; // Convert to MB
          continue;
        }

        // Check for the directory flag
        if (app_params[i] == "-d") { make_directory = true; continue; }

        // Check for a shard prefix
        if (app_params[i] == "-n") {
          shard_prefix = app_params[++i];
          if (shard_prefix.size() < 3) {
            throw std::runtime_error { "Shard names really ought to be at least 3 characters long." };
          }
          continue;
        }

        // Treat this param as a file name instead as it didn't match anythin else
        user_files.emplace_back(app_params[i]);
      } // for
    } catch (...) {
      std::ostringstream msg;
      msg << "You've supplied a bad argument.";
      std::throw_with_nested(std::runtime_error { msg.str() });
    } // try

  }

  // We don't have anything to do on shutdown, but if we did, we could put
  // it here.
  ~app_t() = default;

  // This is the function application operator.  Having this operator allows
  // an instance of this class to be called as though it were a function.
  // This is where we do the work of our application.
  int operator() () {
    int result = 0;

    if (app_params.size() == 0) {
      print_banner();
      print_usage();
      return result;
    }

    // Verbose supplied params
    std::cout << "Supplied Parameters: { size => " << max_shard_size << ", mkdir => "
      << make_directory << ", prefix => '" << shard_prefix << "' }" << std::endl;

    // Verbose user files
    std::cout << "The following were 'files': {" << std::endl;
    bool is_last = false;
    for (std::string file : user_files) {
      if (file == user_files[user_files.size() - 1]) { is_last = true; }
      std::cout << "  " << file << (is_last ? "" : ",") << std::endl;
    }
    std::cout << "}" << std::endl;

    if (user_files.size() == 1) {
      // We have exactly one argument so split it.
      result = split(app_params[0], max_shard_size);
    } else {
      // We have exactly some other number of arguments, so join them.
      result = join(app_params);
    }
    return result;
  }

private:

  // App name and params
  std::string app_name;
  std::vector<std::string> app_params;

  // Any number of files found within the args
  // params are tested if they don't match flags
  // if there is only one file, we try and split
  // otherwise multiple files means we try and join
  std::vector<std::string> user_files;

  // User prefs
  std::string shard_prefix;
  bool make_directory;
  uint64_t max_shard_size;
};  // app_t

// A helper function for printing an exception to the standard error pipe.
// The function recurses to unnest a nested exception completely.
static void print_exception(const std::exception &ex, bool is_first = true) {
  // Write the current exception, prefixed with a separator if this is not
  // our first time through.
  std::cerr << (is_first ? "" : " ") << ex.what();
  // Recurse to print the nested exception, if there is one.
  try {
    std::rethrow_if_nested(ex);
  } catch (const std::exception &nested_ex) {
    print_exception(nested_ex, false);
  }
  // Write an end-of-line and flush the text out to the pipe.
  if (is_first) {
    std::cerr << std::endl;
  }
}

// It's main!
int main(int argc, char *argv[]) {
  int result;
  try {
    // Construct an instance of app_t and call it as a function.  The whole
    // program's run is contained in this single expression.  We will return
    // whatever value the app's function application operator returns.
    result = app_t { argc, argv } ();
  } catch (const std::exception &ex) {
    // Rats, something went wrong.  Print the exception and return the
    // standard error code.
    print_exception(ex);
    result = EXIT_FAILURE;
  }
  return result;
}