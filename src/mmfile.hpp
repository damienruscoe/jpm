#pragma once

#include <cstdint>
#include <string>

class MappedFile {
public:
  using byte_t = uint8_t;
  explicit MappedFile(const std::string &filename);

  MappedFile(const MappedFile &) = delete;
  MappedFile(MappedFile &&) = default;
  MappedFile &operator=(const MappedFile &) = delete;
  MappedFile &operator=(MappedFile &&) noexcept = default;

  ~MappedFile();

  const byte_t *data() const;
  size_t size() const;

private:
  int m_fd = -1;
  const byte_t *m_data = nullptr;
  size_t m_size = 0;
};
