#!/bin/bash
# Comprehensive simulation script for Cache Simulator
# Runs multiple configurations against trace files and compares results

# Default values
SIMULATOR="./build/bin/cachesim"
TRACES_DIR="./traces"
CONFIGS_DIR="./configs"
RESULTS_DIR="./simulation_results"
PARALLEL_JOBS=4
VERBOSE=false
GENERATE_CHARTS=false

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
    echo "  --simulator PATH    Path to cachesim executable (default: $SIMULATOR)"
    echo "  --traces PATH       Path to traces directory (default: $TRACES_DIR)"
    echo "  --configs PATH      Path to configs directory (default: $CONFIGS_DIR)"
    echo "  --results PATH      Path to results directory (default: $RESULTS_DIR)"
    echo "  --jobs N            Number of parallel jobs (default: $PARALLEL_JOBS)"
    echo "  --verbose           Enable verbose output"
    echo "  --charts            Generate performance charts (requires gnuplot)"
    echo "  --help              Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                                    # Run with defaults"
    echo "  $0 --verbose --charts                 # Verbose with charts"
    echo "  $0 --jobs=8 --results=./my_results   # Custom jobs and output"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    key="$1"
    case $key in
        --simulator=*)
            SIMULATOR="${key#*=}"
            shift
            ;;
        --traces=*)
            TRACES_DIR="${key#*=}"
            shift
            ;;
        --configs=*)
            CONFIGS_DIR="${key#*=}"
            shift
            ;;
        --results=*)
            RESULTS_DIR="${key#*=}"
            shift
            ;;
        --jobs=*)
            PARALLEL_JOBS="${key#*=}"
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --charts)
            GENERATE_CHARTS=true
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

print_status $BLUE "Cache Simulator - Comprehensive Simulation Suite"
print_status $BLUE "==============================================="
echo ""

# Validate inputs
if [ ! -f "$SIMULATOR" ]; then
    print_status $RED "Error: Simulator not found at $SIMULATOR"
    print_status $YELLOW "Please build the project first: ./scripts/build_all.sh"
    exit 1
fi

if [ ! -d "$TRACES_DIR" ]; then
    print_status $RED "Error: Traces directory not found at $TRACES_DIR"
    exit 1
fi

if [ ! -d "$CONFIGS_DIR" ]; then
    print_status $RED "Error: Configs directory not found at $CONFIGS_DIR"
    exit 1
fi

# Create results directory
mkdir -p "$RESULTS_DIR"

print_status $GREEN "Configuration:"
echo "  Simulator: $SIMULATOR"
echo "  Traces: $TRACES_DIR"
echo "  Configs: $CONFIGS_DIR"
echo "  Results: $RESULTS_DIR"
echo "  Parallel Jobs: $PARALLEL_JOBS"
echo "  Verbose: $VERBOSE"
echo "  Generate Charts: $GENERATE_CHARTS"
echo ""

# Function to run simulation
run_simulation() {
    local config_file=$1
    local trace_file=$2
    local config_name=$(basename "$config_file" .json)
    local trace_name=$(basename "$trace_file" .txt)
    local output_file="${RESULTS_DIR}/${config_name}_${trace_name}.txt"
    local csv_file="${RESULTS_DIR}/${config_name}_${trace_name}.csv"
    
    if [ "$VERBOSE" = true ]; then
        print_status $YELLOW "Running: $config_name with $trace_name"
    fi
    
    # Run simulation
    "$SIMULATOR" --config "$config_file" "$trace_file" > "$output_file" 2>&1
    
    # Extract key metrics to CSV
    {
        echo "Config,Trace,L1_Hit_Rate,L2_Hit_Rate,Total_Accesses,Processing_Time_ms"
        local l1_hit=$(grep "L1 Hit Ratio" "$output_file" | awk '{print $NF}' | tr -d '%')
        local l2_hit=$(grep "L2 Hit Ratio" "$output_file" | awk '{print $NF}' | tr -d '%')
        local total_acc=$(grep "Total Accesses" "$output_file" | awk '{print $NF}')
        local proc_time=$(grep "Processing Time" "$output_file" | awk '{print $3}')
        echo "$config_name,$trace_name,$l1_hit,$l2_hit,$total_acc,$proc_time"
    } > "$csv_file"
    
    if [ "$VERBOSE" = true ]; then
        print_status $GREEN "  Completed: $config_name with $trace_name"
    fi
}

# Export function for parallel execution
export -f run_simulation
export -f print_status
export SIMULATOR RESULTS_DIR VERBOSE RED GREEN YELLOW BLUE NC

print_status $BLUE "Starting simulations..."
echo ""

# Get list of config and trace files
config_files=("$CONFIGS_DIR"/*.json)
trace_files=("$TRACES_DIR"/*.txt)

# Create job list
job_list="${RESULTS_DIR}/job_list.txt"
> "$job_list"

for config_file in "${config_files[@]}"; do
    for trace_file in "${trace_files[@]}"; do
        echo "$config_file $trace_file" >> "$job_list"
    done
done

total_jobs=$(wc -l < "$job_list")
print_status $YELLOW "Total simulations to run: $total_jobs"
print_status $YELLOW "Using $PARALLEL_JOBS parallel jobs"
echo ""

# Run simulations in parallel
cat "$job_list" | xargs -n 2 -P "$PARALLEL_JOBS" -I {} bash -c 'run_simulation "$@"' _ {}

print_status $GREEN "All simulations completed!"
echo ""

# Combine all CSV results
print_status $BLUE "Combining results..."
combined_csv="${RESULTS_DIR}/combined_results.csv"
echo "Config,Trace,L1_Hit_Rate,L2_Hit_Rate,Total_Accesses,Processing_Time_ms" > "$combined_csv"
for csv_file in "${RESULTS_DIR}"/*.csv; do
    if [ "$(basename "$csv_file")" != "combined_results.csv" ]; then
        tail -n +2 "$csv_file" >> "$combined_csv"
    fi
done

print_status $GREEN "Combined results saved to: $combined_csv"

# Generate performance summary
summary_file="${RESULTS_DIR}/performance_summary.txt"
{
    echo "Cache Simulator Performance Summary"
    echo "==================================="
    echo "Generated: $(date)"
    echo ""
    echo "Total Simulations: $total_jobs"
    echo "Configurations: ${#config_files[@]}"
    echo "Traces: ${#trace_files[@]}"
    echo ""
    echo "Top Performing Configurations (by L1 Hit Rate):"
    tail -n +2 "$combined_csv" | sort -t, -k3 -nr | head -5 | \
        awk -F, '{printf "  %-20s %-20s %6.2f%%\n", $1, $2, $3}'
    echo ""
    echo "Fastest Processing Times:"
    tail -n +2 "$combined_csv" | sort -t, -k6 -n | head -5 | \
        awk -F, '{printf "  %-20s %-20s %8.2f ms\n", $1, $2, $6}'
} > "$summary_file"

print_status $GREEN "Performance summary saved to: $summary_file"

# Generate charts if requested
if [ "$GENERATE_CHARTS" = true ]; then
    if command -v gnuplot &> /dev/null; then
        print_status $BLUE "Generating performance charts..."
        
        # Create gnuplot script for hit rate comparison
        gnuplot_script="${RESULTS_DIR}/generate_charts.gp"
        {
            echo "set terminal png size 1200,800"
            echo "set output '${RESULTS_DIR}/hit_rate_comparison.png'"
            echo "set title 'L1 Cache Hit Rate Comparison'"
            echo "set xlabel 'Configuration'"
            echo "set ylabel 'Hit Rate (%)'"
            echo "set style data histograms"
            echo "set style histogram cluster gap 1"
            echo "set style fill solid border -1"
            echo "set boxwidth 0.9"
            echo "set xtic rotate by -45 scale 0"
            echo "plot '${combined_csv}' using 3:xtic(1) title 'L1 Hit Rate'"
        } > "$gnuplot_script"
        
        gnuplot "$gnuplot_script"
        print_status $GREEN "Chart generated: ${RESULTS_DIR}/hit_rate_comparison.png"
    else
        print_status $YELLOW "Warning: gnuplot not found. Skipping chart generation."
        print_status $YELLOW "Install gnuplot to enable chart generation."
    fi
fi

print_status $BLUE "Simulation Results Summary:"
print_status $BLUE "==========================="
cat "$summary_file"
echo ""
print_status $GREEN "All results saved to: $RESULTS_DIR"
print_status $GREEN "Simulation suite completed successfully!"