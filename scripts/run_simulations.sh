#!/bin/bash

# Cache Simulator Simulation Script
# Runs multiple cache configurations against trace files and compares results

# Colors for better readability
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Check if colors are supported
if [[ -t 1 ]] && [[ $(tput colors) -ge 8 ]]; then
    USE_COLOR=true
else
    USE_COLOR=false
fi

print_colored() {
    local color=$1
    local msg=$2
    
    if $USE_COLOR; then
        echo -e "${color}${msg}${NC}"
    else
        echo "$msg"
    fi
}

print_header() {
    local msg=$1
    print_colored "$BLUE" "============================================="
    print_colored "$BLUE" "$msg"
    print_colored "$BLUE" "============================================="
}

print_subheader() {
    local msg=$1
    print_colored "$CYAN" "---------------------------------------------"
    print_colored "$CYAN" "$msg"
    print_colored "$CYAN" "---------------------------------------------"
}

print_result() {
    local name=$1
    local value=$2
    printf "%-25s: %s\n" "$name" "$value"
}

# Default values
SIMULATOR="./build/bin/cachesim"
TRACES_DIR="./traces"
RESULTS_DIR="./results"
CONFIGS_FILE="./configs.json"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --simulator)
            SIMULATOR="$2"
            shift 2
            ;;
        --traces)
            TRACES_DIR="$2"
            shift 2
            ;;
        --results)
            RESULTS_DIR="$2"
            shift 2
            ;;
        --configs)
            CONFIGS_FILE="$2"
            shift 2
            ;;
        --help)
            echo "Usage: $0 [options]"
            echo "Options:"
            echo "  --simulator PATH  Path to the cache simulator executable (default: ./build/bin/cachesim)"
            echo "  --traces DIR      Directory containing trace files (default: ./traces)"
            echo "  --results DIR     Directory to store results (default: ./results)"
            echo "  --configs FILE    JSON configuration file (default: ./configs.json)"
            echo "  --help            Display this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Check if simulator exists
if [ ! -f "$SIMULATOR" ]; then
    print_colored "$RED" "Error: Simulator executable not found at $SIMULATOR"
    exit 1
fi

# Check if traces directory exists
if [ ! -d "$TRACES_DIR" ]; then
    print_colored "$RED" "Error: Traces directory not found at $TRACES_DIR"
    exit 1
fi

# Create results directory if it doesn't exist
mkdir -p "$RESULTS_DIR"

# Define a set of cache configurations to test
# Format: "name,blocksize,l1_size,l1_assoc,l2_size,l2_assoc,prefetch,prefetch_dist"
CONFIGURATIONS=(
    "baseline,64,32768,4,262144,8,0,0"
    "no_l2,64,32768,4,0,0,0,0"
    "direct_mapped,64,32768,1,262144,4,0,0"
    "larger_l1,64,65536,4,262144,8,0,0"
    "with_prefetch,64,32768,4,262144,8,1,4"
    "adaptive_prefetch,64,32768,4,262144,8,1,8"
    "large_blocks,128,32768,4,262144,8,0,0"
    "fully_featured,64,65536,4,524288,8,1,16"
)

# Find all trace files
TRACE_FILES=$(find "$TRACES_DIR" -name "*.txt" | sort)

if [ -z "$TRACE_FILES" ]; then
    print_colored "$RED" "Error: No trace files found in $TRACES_DIR"
    exit 1
fi

# Print simulation setup
print_header "Cache Simulator Benchmark"
echo "Simulator: $SIMULATOR"
echo "Traces directory: $TRACES_DIR"
echo "Results directory: $RESULTS_DIR"
echo "Number of configurations: ${#CONFIGURATIONS[@]}"
echo "Number of trace files: $(echo "$TRACE_FILES" | wc -l)"
echo ""

# Create CSV header
CSV_FILE="$RESULTS_DIR/benchmark_results.csv"
echo "Trace,Configuration,BlockSize,L1Size,L1Assoc,L2Size,L2Assoc,Prefetch,PrefetchDist,Hits,Misses,Reads,Writes,MissRate,Time" > "$CSV_FILE"

# Run simulations for each trace file and configuration
for TRACE in $TRACE_FILES; do
    TRACE_NAME=$(basename "$TRACE")
    print_header "Processing trace: $TRACE_NAME"
    
    for CONFIG in "${CONFIGURATIONS[@]}"; do
        # Parse configuration
        IFS=',' read -r NAME BLOCKSIZE L1_SIZE L1_ASSOC L2_SIZE L2_ASSOC PREFETCH PREFETCH_DIST <<< "$CONFIG"
        
        print_subheader "Configuration: $NAME"
        echo "  Block Size: $BLOCKSIZE bytes"
        echo "  L1 Cache: $L1_SIZE bytes, $L1_ASSOC-way associative"
        if [ "$L2_SIZE" -gt 0 ]; then
            echo "  L2 Cache: $L2_SIZE bytes, $L2_ASSOC-way associative"
        else
            echo "  L2 Cache: Disabled"
        fi
        echo "  Prefetching: $([ "$PREFETCH" -eq 1 ] && echo "Enabled (distance=$PREFETCH_DIST)" || echo "Disabled")"
        
        # Create output file name
        OUTPUT_FILE="$RESULTS_DIR/${TRACE_NAME%.txt}_${NAME}.txt"
        
        # Run simulation
        echo "  Running simulation..."
        START_TIME=$(date +%s.%N)
        $SIMULATOR "$BLOCKSIZE" "$L1_SIZE" "$L1_ASSOC" "$L2_SIZE" "$L2_ASSOC" "$PREFETCH" "$PREFETCH_DIST" "$TRACE" > "$OUTPUT_FILE" 2>&1
        END_TIME=$(date +%s.%N)
        EXEC_TIME=$(echo "$END_TIME - $START_TIME" | bc)
        
        # Extract results - this will depend on your output format
        # Adjust the grep and awk commands to match your output format
        HITS=$(grep -E "L1 hits|Hits" "$OUTPUT_FILE" | awk '{print $NF}' | head -1)
        MISSES=$(grep -E "L1 misses|Misses" "$OUTPUT_FILE" | awk '{print $NF}' | head -1)
        READS=$(grep -E "L1 reads|Reads" "$OUTPUT_FILE" | awk '{print $NF}' | head -1)
        WRITES=$(grep -E "L1 writes|Writes" "$OUTPUT_FILE" | awk '{print $NF}' | head -1)
        
        # Calculate miss rate
        if [ -n "$HITS" ] && [ -n "$MISSES" ]; then
            TOTAL=$(( HITS + MISSES ))
            MISS_RATE=$(echo "scale=4; $MISSES / $TOTAL" | bc)
        else
            MISS_RATE="N/A"
            print_colored "$YELLOW" "  Warning: Could not extract hit/miss counts from output"
        fi
        
        # Print results
        echo "  Results:"
        print_result "Execution Time" "$(printf "%.3f" "$EXEC_TIME") seconds"
        print_result "Hits" "${HITS:-N/A}"
        print_result "Misses" "${MISSES:-N/A}"
        print_result "Reads" "${READS:-N/A}"
        print_result "Writes" "${WRITES:-N/A}"
        print_result "Miss Rate" "$(printf "%.2f%%" "$(echo "$MISS_RATE * 100" | bc)")"
        
        # Append to CSV
        echo "$TRACE_NAME,$NAME,$BLOCKSIZE,$L1_SIZE,$L1_ASSOC,$L2_SIZE,$L2_ASSOC,$PREFETCH,$PREFETCH_DIST,${HITS:-0},${MISSES:-0},${READS:-0},${WRITES:-0},$MISS_RATE,$EXEC_TIME" >> "$CSV_FILE"
        
        echo ""
    done
done

print_header "Benchmark Complete"
echo "Results saved to $RESULTS_DIR"
echo "CSV summary saved to $CSV_FILE"

# Generate simple analysis if gnuplot is available
if command -v gnuplot &> /dev/null; then
    print_subheader "Generating Performance Charts"
    
    # Create gnuplot script
    GNUPLOT_SCRIPT="$RESULTS_DIR/plot_misses.gp"
    echo "set terminal pngcairo enhanced font 'Arial,12' size 1200,800" > "$GNUPLOT_SCRIPT"
    echo "set output '$RESULTS_DIR/miss_rates.png'" >> "$GNUPLOT_SCRIPT"
    echo "set title 'Cache Miss Rates by Configuration and Trace'" >> "$GNUPLOT_SCRIPT"
    echo "set style data histogram" >> "$GNUPLOT_SCRIPT"
    echo "set style histogram cluster gap 1" >> "$GNUPLOT_SCRIPT"
    echo "set style fill solid border -1" >> "$GNUPLOT_SCRIPT"
    echo "set boxwidth 0.9" >> "$GNUPLOT_SCRIPT"
    echo "set xtic rotate by -45 scale 0" >> "$GNUPLOT_SCRIPT"
    echo "set ylabel 'Miss Rate (%)'" >> "$GNUPLOT_SCRIPT"
    echo "set yrange [0:100]" >> "$GNUPLOT_SCRIPT"
    echo "set datafile separator ','" >> "$GNUPLOT_SCRIPT"
    echo "set key outside" >> "$GNUPLOT_SCRIPT"
    
    # Different colors for each configuration
    echo "set palette defined (0 '#1f77b4', 1 '#ff7f0e', 2 '#2ca02c', 3 '#d62728', 4 '#9467bd', 5 '#8c564b', 6 '#e377c2', 7 '#7f7f7f')" >> "$GNUPLOT_SCRIPT"
    
    # Create the plot command
    echo -n "plot " >> "$GNUPLOT_SCRIPT"
    CONFIG_COUNT=${#CONFIGURATIONS[@]}
    for ((i=0; i<CONFIG_COUNT; i++)); do
        IFS=',' read -r NAME _ <<< "${CONFIGURATIONS[$i]}"
        echo -n "'$CSV_FILE' using (\$13*100):xtic(1) every ::1 title '$NAME' lc palette frac ($i/$CONFIG_COUNT)" >> "$GNUPLOT_SCRIPT"
        if [ $i -lt $((CONFIG_COUNT-1)) ]; then
            echo -n ", " >> "$GNUPLOT_SCRIPT"
        fi
    done
    
    # Execute gnuplot
    gnuplot "$GNUPLOT_SCRIPT"
    echo "Charts generated: $RESULTS_DIR/miss_rates.png"
else
    print_colored "$YELLOW" "Gnuplot not found. Skipping chart generation."
fi

echo ""
print_colored "$GREEN" "All simulations completed successfully!"