# C Project

A simple C project with proper project structure and build system.

## Project Structure

```
.
├── Makefile          # Build automation
├── main.c           # Main source file
├── .gitignore      # Git ignore rules
└── README.md       # This file
```

## Building the Project

To build the project, run:

```bash
make
```

To clean build artifacts:

```bash
make clean
```

## Running the Program

After building, run the program with:

```bash
./program
```

## Development

- The project uses GCC compiler
- Build flags include `-Wall -Wextra -g` for warnings and debug information
- Source files are compiled into object files and then linked into the final executable
