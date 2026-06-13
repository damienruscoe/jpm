#pragma once

#include <fcntl.h>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

class MappedFile {
public:
  explicit MappedFile(const std::string &filename);
  ~MappedFile();

  const char *data() const;
  size_t size() const;
  bool is_open() const;

private:
  int fd_ = -1;
  const char *data_ = nullptr;
  size_t size_ = 0;
};
