// copyright 2026 coletm

#include"proj3/lib/include/mmap.h"
#include<iostream>
#include<string>
#include<cstring>

void print_usage() {
  std::cerr << "valid usages:\n"
      << "mmap_util create <path> <fill_char> <size>\n"
      << "mmap_util insert <path> <offset> <bytes_incoming> < stdin\n"
      << "mmap_util append <path> <bytes_incoming> < stdin" << std::endl;
}

int create(const char* path, char fill_char, std::size_t size) {
  int fd = proj3::open(path, proj3::O_RDWR | proj3::O_CREAT | proj3::O_TRUNC,
                        0664);

  proj3::ftruncate(fd, static_cast<off_t>(size));
  void* map = proj3::mmap(nullptr, size, proj3::PROT_READ | proj3::PROT_WRITE,
                          proj3::MAP_SHARED, fd, 0);

  std::memset(map, fill_char, size);
  proj3::msync(map, size, proj3::MS_SYNC);
  proj3::munmap(map, size);
  proj3::close(fd);
  
  return 0;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    print_usage();
    return 1;
  }

  std::string cmd = argv[1];

  if (cmd == "create") {
    // grab arguments from command line
    const char* path = argv[2];
    // if user enters more than one character, just use the first one
    char fill_char = argv[3][0];
    std::size_t size = std::stoul(argv[4]);
    return create(path, fill_char, size);
  } else if (cmd == "insert") {
    // TODO
  } else if (cmd == "append") {
    // TODO
  } else {
    print_usage();
    return 1;
  }

  return 0;
}
