#pragma once

#include <string>

class MappedFile {
public:
  explicit MappedFile(const std::string &filename);
  ~MappedFile();

  const char *data() const;
  size_t size() const;

private:
  int fd_ = -1;
  const char *data_ = nullptr;
  size_t size_ = 0;
};
