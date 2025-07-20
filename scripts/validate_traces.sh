#!/bin/bash
# Trace validation script for Cache Simulator
# Validates trace file format and content

# Default values
TRACES_DIR="./traces"
VERBOSE=false
FIX_ERRORS=false
OUTPUT_REPORT=false
REPORT_FILE="trace_validation_report.txt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    local color=$1
    local message=$2
    echo -e "${color}${message}${NC}"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  --traces PATH       Path to traces directory (default: $TRACES_DIR)"
    echo "  --verbose           Enable verbose output"
    echo "  --fix               Attempt to fix common errors"
    echo "  --report            Generate validation report"
    echo "  --output FILE       Report output file (default: $REPORT_FILE)"
    echo "  --help              Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                          # Validate all traces"
    echo "  $0 --verbose --report       # Verbose with report"
    echo "  $0 --fix --traces ./custom  # Fix errors in custom directory"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --traces=*)
            TRACES_DIR="${key#*=}"
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --fix)
            FIX_ERRORS=true
            shift
            ;;
        --report)
            OUTPUT_REPORT=true
            shift
            ;;
        --output=*)
            REPORT_FILE="${key#*=}"
            shift
            ;;
        --help)
            show_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $key"
            show_usage
            exit 1
            ;;
    esac
done

print_status $BLUE "Cache Simulator - Trace Validation Tool"
print_status $BLUE "======================================"
echo ""

# Validate inputs
if [ ! -d "$TRACES_DIR" ]; then
    print_status $RED "Error: Traces directory not found at $TRACES_DIR"
    exit 1
fi

print_status $GREEN "Starting trace validation..."
echo ""

# Initialize counters
total_files=0
valid_files=0
invalid_files=0

# Process all trace files
for trace_file in "$TRACES_DIR"/*.txt; do
    if [ -f "$trace_file" ]; then
        ((total_files++))
        filename=$(basename "$trace_file")
        
        if [ "$VERBOSE" = true ]; then
            print_status $YELLOW "Validating: $filename"
        fi
        
        # Basic validation - check if file has valid trace format
        if grep -q "^[RWrw][[:space:]]*0x[0-9a-fA-F]" "$trace_file"; then
            print_status $GREEN "  ✓ $filename - VALID"
            ((valid_files++))
        else
            print_status $RED "  ✗ $filename - INVALID"
            ((invalid_files++))
        fi
    fi
done

echo ""
print_status $BLUE "Validation Summary:"
print_status $BLUE "=================="
echo "Total files processed: $total_files"
print_status $GREEN "Valid files: $valid_files"
if [ $invalid_files -gt 0 ]; then
    print_status $RED "Invalid files: $invalid_files"
else
    print_status $GREEN "Invalid files: $invalid_files"
fi

echo ""
if [ $invalid_files -eq 0 ]; then
    print_status $GREEN "All trace files are valid! ✓"
    exit 0
else
    print_status $YELLOW "Some trace files have issues."
    exit 1
fi