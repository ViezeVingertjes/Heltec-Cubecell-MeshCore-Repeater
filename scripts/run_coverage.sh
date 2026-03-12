#!/usr/bin/env bash
# Run native tests with coverage and generate lcov report.
# Requires: pio, gcov, lcov (e.g. apt install lcov)
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
cd "$PROJECT_DIR"
echo "Building and running tests with coverage..."
pio test -e native_coverage
BUILD_DIR=".pio/build/native_coverage"
if command -v lcov >/dev/null 2>&1; then
  echo "Collecting coverage..."
  lcov -d "$BUILD_DIR" -c -o lcov.info --rc lcov_branch_coverage=0 2>/dev/null || true
  lcov -r lcov.info '*/Unity/*' '*/test/*' -o lcov.info --rc lcov_branch_coverage=0 2>/dev/null || true
  genhtml -o cov lcov.info --rc lcov_branch_coverage=0 2>/dev/null || true
  echo "Report: file://$PROJECT_DIR/cov/index.html"
else
  echo "lcov not found. Install with: apt install lcov (or equivalent)"
  echo "Raw .gcno/.gcda files are in $BUILD_DIR"
fi
