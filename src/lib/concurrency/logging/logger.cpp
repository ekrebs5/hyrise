#include "logger.hpp"
#include "abstract_logger.hpp"
#include "initial_logger.hpp"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sstream>

namespace opossum {

AbstractLogger& Logger::getInstance() {
  static InitialLogger instance;
  return instance;
}

}  // namespace opossum
