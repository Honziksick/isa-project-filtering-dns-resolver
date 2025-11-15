#!/bin/bash

set -euo pipefail

# Cesty
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VENV_DIR="$SCRIPT_DIR/myenv"

rm -rf "$SCRIPT_DIR/test_filter_copies" "$SCRIPT_DIR/.pytest_cache"

echo "=== DNS Resolver Integration Tests ==="

# Parsing arguments
PYTEST_FLAGS="-v --tb=short"
STDERR_REDIRECT=""
USE_VALGRIND=""
KEEP_VENV="false"

while [[ $# -gt 0 ]]; do
    case $1 in
        --stdout)
            PYTEST_FLAGS="-s -v --tb=short"
            STDERR_REDIRECT="2>/dev/null"
            shift
            ;;
        --verbose)
            PYTEST_FLAGS="-s -v --tb=short"
            STDERR_REDIRECT="2>/dev/null"
            shift
            ;;
        --keep-venv)
            KEEP_VENV="true"
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--stdout|--verbose] [--keep-venv]"
            exit 1
            ;;
    esac
done

# Ensure venv exists
if [ ! -d "$VENV_DIR" ]; then
    echo "Creating virtual environment in $VENV_DIR ..."
    python3 -m venv "$VENV_DIR"
fi

VENV_PY="$VENV_DIR/bin/python"

# Check/install requirements
echo "Checking Python dependencies in venv..."
if ! "$VENV_PY" -c "import pytest, scapy, dns" >/dev/null 2>&1; then
    echo "Installing/upgrading pip, setuptools, wheel..."
    "$VENV_PY" -m pip install --upgrade pip setuptools wheel
    if [ -f "$SCRIPT_DIR/IntegrationTests/requirements.txt" ]; then
        echo "Installing IntegrationTests/requirements.txt..."
        "$VENV_PY" -m pip install -r "$SCRIPT_DIR/IntegrationTests/requirements.txt"
    else
        echo "Warning: requirements file not found: $SCRIPT_DIR/IntegrationTests/requirements.txt"
    fi
fi

# Helper to run pytest through venv python
run_pytest() {
    local testfile="$1"
    echo "=== Running: $testfile ==="
    eval "\"$VENV_PY\" -m pytest \"$SCRIPT_DIR/$testfile\" $PYTEST_FLAGS $USE_VALGRIND $STDERR_REDIRECT"
}

echo "Running integration tests..."

run_pytest "IntegrationTests/BasicFunctionalityTest.py"
run_pytest "IntegrationTests/ComplexFunctionalityTest.py"
run_pytest "IntegrationTests/ErrorHandlingTest.py"
run_pytest "IntegrationTests/CompressedNamesTest.py"

echo "=== Integration tests completed! ==="
