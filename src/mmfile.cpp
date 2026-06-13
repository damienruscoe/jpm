#include "mmfile.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

MappedFile::MappedFile(const std::string &filename) {
  m_fd = open(filename.c_str(), O_RDONLY);
  if (m_fd == -1)
    return;

  struct stat st;
  if (fstat(m_fd, &st) == -1) {
    close(m_fd);
    m_fd = -1;
    return;
  }

  m_size = st.st_size;
  m_data = static_cast<const char *>(
      mmap(nullptr, m_size, PROT_READ, MAP_PRIVATE, m_fd, 0));
  if (m_data == MAP_FAILED) {
    m_data = nullptr;
    close(m_fd);
    m_fd = -1;
  }
}

MappedFile::~MappedFile() {
  if (m_data)
    munmap(const_cast<char *>(m_data), m_size);
  if (m_fd != -1)
    close(m_fd);
}

const char *MappedFile::data() const { return m_data; }

size_t MappedFile::size() const { return m_size; }
