#include "../src/mmfile.hpp"
#include <fcntl.h>
#include <fstream>
#include <gtest/gtest.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

TEST(MappedFileTest, MapFile) {
  std::string filename = "test_map.txt";
  std::string content = "Hello, world!";

  // Create a temporary file
  std::ofstream ofs(filename);
  ofs << content;
  ofs.close();

  // Map the file
  MappedFile mf(filename);

  EXPECT_EQ(mf.size(), content.size());
  EXPECT_EQ(std::string(reinterpret_cast<const char *>(mf.data()), mf.size()),
            content);

  // Clean up
  std::remove(filename.c_str());
}

TEST(MappedFileTest, OpenFailed) {
  MappedFile mf("non_existent_file.txt");

  EXPECT_EQ(mf.data(), nullptr);
  EXPECT_EQ(mf.size(), 0);
}
