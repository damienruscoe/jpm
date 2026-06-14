#pragma once

#include <string>

class MappedFile {
public:
  explicit MappedFile(const std::string &filename);

  MappedFile(const MappedFile &) = delete;
  MappedFile(MappedFile &&) = default;
  MappedFile &operator=(const MappedFile &) = delete;
  MappedFile &operator=(MappedFile &&) noexcept = default;

  ~MappedFile();

  const char *data() const;
  size_t size() const;

private:
  int m_fd = -1;
  const char *m_data = nullptr;
  size_t m_size = 0;
};
