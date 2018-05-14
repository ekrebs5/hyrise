#include "abstract_logger.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>

namespace opossum {

void AbstractLogger::_write_to_logfile(const std::stringstream& ss) {
  _mutex.lock();
  write(_file_descriptor, (void*)ss.str().c_str(), ss.str().length());
  _mutex.unlock();
}

void AbstractLogger::flush() {
  fsync(_file_descriptor);
}

AbstractLogger::AbstractLogger(){
  std::string directory = "/Users/Dimitri/";
  std::string filename = directory + "hyrise-log.txt";

  // TODO: what if directory does not exists?

  int oflags = O_WRONLY | O_APPEND | O_CREAT;

  // read and write rights needed, since default rights do not allow to reopen the file after restarting the db
  mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  _file_descriptor = open(filename.c_str(), oflags, mode);

  DebugAssert(_file_descriptor != -1, "Logfile could not be opened or created: " + filename);
}

}  // namespace opossum
