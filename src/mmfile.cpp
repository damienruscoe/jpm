#include "mmfile.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

MappedFile::MappedFile(const std::string &filename) {
  fd_ = open(filename.c_str(), O_RDONLY);
  if (fd_ == -1)
    return;

  struct stat st;
  if (fstat(fd_, &st) == -1) {
    close(fd_);
    fd_ = -1;
    return;
  }

  size_ = st.st_size;
  data_ = static_cast<const char *>(
      mmap(nullptr, size_, PROT_READ, MAP_PRIVATE, fd_, 0));
  if (data_ == MAP_FAILED) {
    data_ = nullptr;
    close(fd_);
    fd_ = -1;
  }
}

MappedFile::~MappedFile() {
  if (data_)
    munmap(const_cast<char *>(data_), size_);
  if (fd_ != -1)
    close(fd_);
}

const char *MappedFile::data() const { return data_; }

size_t MappedFile::size() const { return size_; }
