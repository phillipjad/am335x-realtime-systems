# Repository for SWE660 pair projects

## Partner Names
- Phillip Miavelstuck (pdickey@gmu.edu)
- Kabir Jolly (kjolly2@gmu.edu)

## How to build assignments
- Run the command `./build.sh -t p[1..n]` or `./build.sh -t project[1..n]`
- To enable debug mode, add '-d' or '--debug' to build command
- To read from a project's configuration file instead of from stdio, add '-u' or '--use-config' to build command
- To run the program using mmap, rather than i/o add '-m' or '--mmap` to build command
- The project executable will be output into the `build/` directory

## Recommended aliases (Add to ~/.bashrc)
- Run pre-commit: `alias pcrun='pre-commit run --all-files'`
