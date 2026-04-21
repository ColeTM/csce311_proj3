# CSCE311 Project 3 -- Memory-Mapped File Manipulation

This project's goal is to work directly with memory-mapped files using `mmap`. The task was to implement a command-line tool that performs file operations entirely through memory mapping, and without an read/write system calls directly on the target file.

## Source code file contents

All of the source code for the command line tool is written in `src/main.cc`. Its contents include:

- `create()` function: creates a new file at a given path with the size of a given number of bytes
- `insert()` function: inserts some bytes at a given offset in a file. if there is an error, it restores the original file
- `append()` function: appends some bytes to the end of a file
- `main()` function: executes the desired function based on the command line call

## Usage

- Create: `bin/mmap_util create <path> <fill_char> <size>`
- Insert: `bin/mmap_util insert <path> <offset> <bytes_incoming> < stdin`
- Append: `bin/mmap_util append <path> <bytes_incoming> < stdin`

Example execution:

```bash
bin/mmap_util create dat/data.bin A 1024
echo -n "hello" | bin/mmap_util insert dat/data.bin 100 4
bin/mmap_util append dat/data.bin 1027 < dat/data_app.bin
```
