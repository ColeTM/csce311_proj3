// copyright 2026 coletm

#include<unistd.h>
#include<sys/stat.h>
#include<iostream>
#include<string>
#include<cstring>
#include<vector>
#include<algorithm>
#include"proj3/lib/include/mmap.h"

// helper function to print usage message
void print_usage() {
  std::cerr << "valid usages:\n"
      << "mmap_util create <path> <fill_char> <size>\n"
      << "mmap_util insert <path> <offset> <bytes_incoming> < stdin\n"
      << "mmap_util append <path> <bytes_incoming> < stdin" << std::endl;
}

// helper function to check if the correct number of bytes are being read
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

// creates a new file at the given path of 'size' bytes.
// filled entirely with 'fill_char', a single byte
int create(const char* path, char fill_char, std::size_t size) {
  int fd = proj3::open(path, proj3::O_RDWR | proj3::O_CREAT | proj3::O_TRUNC,
                        0664);
  // set the file size
  proj3::ftruncate(fd, static_cast<off_t>(size));
  void* map = proj3::mmap(nullptr, size, proj3::PROT_READ | proj3::PROT_WRITE,
                          proj3::MAP_SHARED, fd, 0);
  // write to map/file
  std::memset(map, fill_char, size);
  proj3::msync(map, size, proj3::MS_SYNC);
  proj3::munmap(map, size);
  proj3::close(fd);

  return 0;
}

// inserts 'bytes_incoming' bytes at 'offset' in the file at 'path'
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

  try {
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
    // if an error occurs, restore the original file
  } catch (const std::exception& e) {
    int fd2 = proj3::open(path, proj3::O_RDWR | proj3::O_TRUNC);
    proj3::ftruncate(fd2, static_cast<off_t>(old_size));
    void* map2 = proj3::mmap(nullptr, old_size, proj3::PROT_READ |
                              proj3::PROT_WRITE, proj3::MAP_SHARED, fd2, 0);

    std::memcpy(map2, backup.data(), old_size);
    proj3::msync(map2, old_size, proj3::MS_SYNC);
    proj3::munmap(map2, old_size);
    proj3::close(fd2);
    proj3::close(fd);
    return 1;
  }
  return 0;
}

// appends 'bytes_incoming' to the end of the file at 'path'
int append(const char* path, std::size_t bytes_incoming) {
  // open file and get its current size
  int fd = proj3::open(path, proj3::O_RDWR);
  struct stat f_stat;
  proj3::fstat(fd, &f_stat);
  std::size_t file_size = static_cast<std::size_t>(f_stat.st_size);

  std::size_t original_size = file_size;
  std::size_t remaining = bytes_incoming;
  std::size_t write_offset = file_size;

  // continue appending until all new bytes have been written
  while (remaining > 0) {
    // each addition being written at a time is either all of the new bytes, or
    // equal to the current file size if its size would be more than doubled.
    // min addition size of one page so that if current file size is 0
    // we can still start appending
    std::size_t addition = std::min(remaining,
                                    std::max(file_size, std::size_t(4096)));
    // adjust file size
    std::size_t new_size = file_size + addition;
    proj3::ftruncate(fd, static_cast<off_t>(new_size));
    // map the new portion
    void* map = proj3::mmap(nullptr, new_size, proj3::PROT_READ |
                            proj3::PROT_WRITE, proj3::MAP_SHARED, fd, 0);
    uint8_t* data = static_cast<uint8_t*>(map);
    // read the new bytes into the mapped region
    std::size_t total_read = 0;
    while (total_read < addition) {
      int rec = ::read(STDIN_FILENO, data + write_offset + total_read,
                        addition - total_read);
      // check for error
      if (rec <= 0) {
        proj3::msync(map, new_size, proj3::MS_SYNC);
        proj3::munmap(map, new_size);
        proj3::ftruncate(fd, static_cast<off_t>(original_size));
        proj3::close(fd);
        std::cerr << "error: stdin ended before " << bytes_incoming
          << " bytes were read" << std::endl;
        return 1;
      }
      total_read += rec;
    }
    proj3::msync(map, new_size, proj3::MS_SYNC);
    proj3::munmap(map, new_size);
    // update relevant values for next iteration
    write_offset += addition;
    file_size = new_size;
    remaining -= addition;
  }
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
    const char* path = argv[2];
    std::size_t bytes_incoming = std::stoul(argv[3]);
    return append(path, bytes_incoming);
  } else {
    print_usage();
    return 1;
  }

  return 0;
}
