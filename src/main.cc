// copyright 2026 coletm

#include"proj3/lib/include/mmap.h"
#include<unistd.h>
#include<sys/stat.h>
#include<iostream>
#include<string>
#include<cstring>
#include<vector>

// helper function to print usage message
void print_usage() {
  std::cerr << "valid usages:\n"
      << "mmap_util create <path> <fill_char> <size>\n"
      << "mmap_util insert <path> <offset> <bytes_incoming> < stdin\n"
      << "mmap_util append <path> <bytes_incoming> < stdin" << std::endl;
}

bool read_stdin(std::vector<uint8_t>& buf, std::size_t n) {
  buf.resize(n);
  std::size_t total = 0;
  while (total < n) {
    int rec = ::read(STDIN_FILENO, buf.data() + total, n - total);
    if (rec <= 0)
      return false;
    total += rec;
  }
  return true;
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

int insert(const char* path, std::size_t offset, std::size_t bytes_incoming) {
  std::vector<uint8_t> incoming;
  if (!read_stdin(incoming, bytes_incoming)) {
    std::cerr << "error: stdin ended before " << bytes_incoming
      << "bytes were read" << std::endl;
    return 1;
  }
  
  // open up file and get its size
  int fd = proj3::open(path, proj3::O_RDWR);
  struct stat f_stat;
  proj3::fstat(fd, &f_stat);
  std::size_t old_size = static_cast<std::size_t>(f_stat.st_size);
  
  // make sure that offset is not greater than file size
  if (offset > old_size) {
    std::cerr << "error: offset " << offset << " exceeds file size "
      << old_size << std::endl;
    proj3::close(fd);
    return 1;
  }
  
  // make backup of file to restore in case of error
  std::vector<uint8_t> backup(old_size);
  void* temp = proj3::mmap(nullptr, old_size, proj3::PROT_READ,
                                    proj3::MAP_SHARED, fd, 0);
  std::memcpy(backup.data(), temp, old_size);
  proj3::munmap(temp, old_size);
  
  // resize file to fit added contents
  std::size_t new_size = old_size + bytes_incoming;
  proj3::ftruncate(fd, static_cast<off_t>(new_size));
  
  // map newly sized file
  void* map = proj3::mmap(nullptr, new_size, proj3::PROT_READ |
                          proj3::PROT_WRITE, proj3::MAP_SHARED, fd, 0);
  uint8_t* data = static_cast<uint8_t*>(map);
  
  // move data after the insert to its new location
  std::memmove(data + offset + bytes_incoming, data + offset,
                old_size - offset);
  // insert new content into the gap
  std::memcpy(data + offset, incoming.data(), bytes_incoming);
  // sync, unmap, and close the file
  proj3::msync(map, new_size, proj3::MS_SYNC);
  proj3::munmap(map, new_size);
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
    const char* path = argv[2];
    std::size_t offset = std::stoul(argv[3]);
    std::size_t bytes_incoming = std::stoul(argv[4]);
    return insert(path, offset, bytes_incoming);
  } else if (cmd == "append") {
    // TODO
  } else {
    print_usage();
    return 1;
  }

  return 0;
}
