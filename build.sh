#!/usr/bin/env bash

# Set e to instantly fail when errors occurs on commands
# Set u to make unset variables an error
set -eu

### Global Variables
TARGET=""                      # Which project to build
CMAKE_ARGS=("--DEXAMPLE=HELO") # Arguments for cmake

### Functions

help() {
	echo "Usage: $(basename "$0") [options]"
	echo ""
	echo "Options:"
	echo "  -t, --target <value>   Set the build target"
	echo "  -h, --help             Show this help message and exit with status code 0"
	echo ""
	echo "Examples:"
	echo "  ./build.sh --target project1"
	echo "  ./build.sh -t project2"
	exit 0
}

# Parse command-line arguments
parse_arguments() {
	while [[ $# -gt 0 ]]; do
		case "$1" in
		-t | --target)
			TARGET="$2"
			shift 2
			;;
		-h | --help)
			help
			;;
		*)
			echo "Error: Unknown argument '$1'" >&2
			exit 1
			;;
		esac
	done
}

### Main script logic

# Parse arguments
parse_arguments "$@"

# Check that provided target maps to a valid assignment
case $TARGET in
"p1" | "project1")
	echo "Building project 1"
	CMAKE_ARGS+=("--DTARGET=P1")
	;;
*)
	echo "Error: Unknown target '$TARGET' provided" >&2
	exit 1
	;;
esac

cmake --build "${CMAKE_ARGS[@]}"
