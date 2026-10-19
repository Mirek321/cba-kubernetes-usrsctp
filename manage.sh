#!/usr/bin/env bash

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"

function build() {
    echo "==> Configuring project..."
    cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR"

    echo "==> Building project..."
    cmake --build "$BUILD_DIR" -j"$(nproc)"
}

function rebuild() {
    echo "==> Removing build directory..."
    rm -rf "$BUILD_DIR"

    build
}

function run_client() {
    echo "==> Starting client..."
    "$BUILD_DIR/bin/client" "$@"
}

function run_server() {
    echo "==> Starting server..."
    "$BUILD_DIR/bin/server" "$@"
}

function help() {
    echo "Usage: ./manage.sh <command> [arguments]"
    echo
    echo "Commands:"
    echo "  build       Configure and build project"
    echo "  rebuild     Clean build and build project from scratch"
    echo "  client      Run client"
    echo "  server      Run server"
    echo "  help        Show this help"
}

case "${1:-help}" in
    build)
        build
        ;;

    rebuild)
        rebuild
        ;;

    client)
        shift
        run_client "$@"
        ;;

    server)
        shift
        run_server "$@"
        ;;

    help|-h|--help)
        help
        ;;

    *)
        echo "Unknown command: $1"
        echo
        help
        exit 1
        ;;
esac

