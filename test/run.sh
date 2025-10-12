#!/bin/bash

set -e
rm -rf test_filter_copies .pytest_cache

echo "=== DNS Resolver Integration Tests ==="

# Parsing arguments
PYTEST_FLAGS="-v --tb=short"
STDERR_REDIRECT=""
USE_VALGRIND=""

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
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--stdout|--verbose] [--valgrind]"
            exit 1
            ;;
    esac
done

# Requirements check
echo "Checking requirements..."
python3 -c "import pytest, scapy, dns" || {
    echo "Missing Python dependencies. Install with:"
    echo "pip install -r IntegrationTests/requirements.txt"
    exit 1
}

# Run tests
echo "Running integration tests..."

# Basic functionality testy
echo "=== Basic Functionality Tests ==="
eval "python3 -m pytest IntegrationTests/BasicFunctionalityTest.py $PYTEST_FLAGS $USE_VALGRIND $STDERR_REDIRECT"

# Complex functionality testy
echo "=== Complex Functionality Tests ==="
eval "python3 -m pytest IntegrationTests/ComplexFunctionalityTest.py $PYTEST_FLAGS $USE_VALGRIND $STDERR_REDIRECT"

# Error handling testy
echo "=== Error Handling Tests ==="
eval "python3 -m pytest IntegrationTests/ErrorHandlingTest.py $PYTEST_FLAGS $USE_VALGRIND $STDERR_REDIRECT"

# Všechny testy najednou
echo "=== All Integration Tests ==="
eval "python3 -m pytest IntegrationTests/ $PYTEST_FLAGS $USE_VALGRIND \
    --resolver-binary=../dns \
    --upstream-dns=8.8.8.8 \
    --test-port=15353 \
    --timeout=30 $STDERR_REDIRECT"

echo "=== Integration tests completed! ==="
