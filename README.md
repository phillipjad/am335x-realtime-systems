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
- To build for various architectures, use the `-a` or `--arch` flag.
    - To build p2 for ARM -> `./build.sh -t p2 -a arm -c`
    - To build p2 for x86_64 -> `./build.sh -t p2 -a x86_64 -c`
    - etc...
 
## How to run assignments
- After building run the produced executable in sudo. E.g. for project 2 -> `sudo ./p2`

## Default dev pin assignments
- Default pin assignments for each project can be found in the corrsponding project config -> p2 default pin configuration can be found in [p2_config.cfg](https://github.com/phillipjad/swe660-s26-jolly-miavelstuck/blob/main/config/p2_config.cfg)

## Recommended aliases (Add to ~/.bashrc)
- Run pre-commit: `alias pcrun='pre-commit run --all-files'`
