#!/usr/bin/env python3
"""
Ecosystem Simulation Benchmark Script

This script runs performance benchmarks on the ecosystem simulation across different
input sizes and thread counts, then generates performance scaling plots.
"""

import subprocess
import time
import json
import os
import sys
import matplotlib.pyplot as plt
import numpy as np
from pathlib import Path

# Configuration
INPUT_SIZES = ['5x5', '10x10', '20x20', '100x100', '100x100_unbal01', '100x100_unbal02', '200x200']
THREAD_COUNTS = [0, 2, 4, 8, 16, 25]  # 0 = sequential
EXECUTABLE = './ecosystem'
ECOSYSTEM_DIR = 'ecosystem_examples'
RESULTS_FILE = 'benchmark_results.json'
PLOTS_DIR = 'benchmark_plots'

class BenchmarkRunner:
    def __init__(self):
        self.results = {}
        self.ensure_executable()
        self.create_plots_dir()
    
    def ensure_executable(self):
        """Ensure the ecosystem executable exists and is built"""
        if not os.path.exists(EXECUTABLE):
            print("Building ecosystem executable...")
            result = subprocess.run(['make'], capture_output=True, text=True)
            if result.returncode != 0:
                print(f"Build failed: {result.stderr}")
                sys.exit(1)
            print("Build completed successfully")
    
    def create_plots_dir(self):
        """Create directory for plots if it doesn't exist"""
        Path(PLOTS_DIR).mkdir(exist_ok=True)
    
    def get_input_file(self, input_size):
        """Get the input file path for a given input size"""
        return f"{ECOSYSTEM_DIR}/input{input_size}"
    
    def run_single_benchmark(self, input_size, thread_count, runs=3):
        """Run a single benchmark configuration multiple times and return average time"""
        input_file = self.get_input_file(input_size)
        
        if not os.path.exists(input_file):
            print(f"Warning: Input file {input_file} not found, skipping...")
            return None
        
        times = []
        print(f"  Running {input_size} with {thread_count if thread_count > 0 else 'sequential'} thread(s)...", end=' ')
        
        for run in range(runs):
            try:
                # Run the simulation and parse execution time from output
                with open(input_file, 'r') as f:
                    result = subprocess.run(
                        [EXECUTABLE, str(thread_count)],
                        stdin=f,
                        capture_output=True,
                        text=True,
                        timeout=300  # 5 minute timeout
                    )
                
                if result.returncode != 0:
                    print(f"\nError in run {run+1}: {result.stderr}")
                    continue
                
                # Parse execution time from the output for parallel runs only
                # Sequential runs don't output timing info, so we use process timing
                execution_time = None
                
                if thread_count > 0:  # Parallel execution
                    for line in result.stdout.split('\n'):
                        if 'Took' in line and 'microseconds' in line:
                            try:
                                # Extract microseconds from "Took X microseconds"
                                parts = line.split()
                                microseconds_idx = parts.index('microseconds')
                                microseconds = int(parts[microseconds_idx - 1])
                                execution_time = microseconds / 1_000_000  # Convert to seconds
                                break
                            except (ValueError, IndexError):
                                continue
                
                # For sequential or if timing not found, measure process time
                if execution_time is None:
                    # Measure actual execution time excluding I/O
                    start_time = time.perf_counter()
                    with open(input_file, 'r') as f:
                        process_result = subprocess.run(
                            [EXECUTABLE, str(thread_count)],
                            stdin=f,
                            stdout=subprocess.DEVNULL,  # Suppress output for timing
                            stderr=subprocess.PIPE,
                            text=True,
                            timeout=300
                        )
                    end_time = time.perf_counter()
                    
                    if process_result.returncode != 0:
                        print(f"\nError in timing run {run+1}: {process_result.stderr}")
                        continue
                        
                    execution_time = end_time - start_time
                
                times.append(execution_time)
                print(f"{execution_time:.3f}s", end=' ')
                
            except subprocess.TimeoutExpired:
                print(f"\nTimeout in run {run+1}")
                continue
            except Exception as e:
                print(f"\nException in run {run+1}: {e}")
                continue
        
        if not times:
            print("FAILED")
            return None
        
        avg_time = sum(times) / len(times)
        std_dev = np.std(times) if len(times) > 1 else 0
        print(f"-> avg: {avg_time:.3f}s ±{std_dev:.3f}s")
        
        return {
            'average': avg_time,
            'std_dev': std_dev,
            'times': times,
            'runs': len(times)
        }
    
    def run_all_benchmarks(self):
        """Run benchmarks for all input sizes and thread counts"""
        print("Starting comprehensive benchmark...")
        print("=" * 60)
        
        for input_size in INPUT_SIZES:
            print(f"\nBenchmarking {input_size}:")
            self.results[input_size] = {}
            
            # Determine appropriate thread counts for this input size
            max_threads = self.get_max_threads(input_size)
            thread_counts = [t for t in THREAD_COUNTS if t <= max_threads]
            
            for thread_count in thread_counts:
                result = self.run_single_benchmark(input_size, thread_count)
                if result:
                    label = 'sequential' if thread_count == 0 else f'{thread_count}_threads'
                    self.results[input_size][label] = result
        
        print("\n" + "=" * 60)
        print("Benchmark completed!")
    
    def get_max_threads(self, input_size):
        """Get maximum reasonable thread count for input size"""
        # Thread counts should not exceed the number of rows in the input
        # Based on the makefile test patterns
        size_map = {
            '5x5': 4,           # 5 rows -> max 4 threads  
            '10x10': 8,         # 10 rows -> max 8 threads
            '20x20': 16,        # 20 rows -> max 16 threads
            '100x100': 25,      # 100 rows -> max 25 threads (from makefile)
            '100x100_unbal01': 25,
            '100x100_unbal02': 25, 
            '200x200': 25       # 200 rows -> max 25 threads
        }
        return size_map.get(input_size, 50)
    
    def save_results(self):
        """Save benchmark results to JSON file"""
        with open(RESULTS_FILE, 'w') as f:
            json.dump(self.results, f, indent=2)
        print(f"Results saved to {RESULTS_FILE}")
    
    def load_results(self):
        """Load benchmark results from JSON file"""
        if os.path.exists(RESULTS_FILE):
            with open(RESULTS_FILE, 'r') as f:
                self.results = json.load(f)
            print(f"Results loaded from {RESULTS_FILE}")
            return True
        return False
    
    def create_performance_plots(self):
        """Create performance scaling plots"""
        if not self.results:
            print("No results available for plotting")
            return
        
        print("\nGenerating performance plots...")
        
        # Plot 1: Individual input size scaling
        self.plot_individual_scaling()
        
        # Plot 2: Comparative scaling across input sizes
        self.plot_comparative_scaling()
        
        # Plot 3: Speedup analysis
        self.plot_speedup_analysis()
        
        # Plot 4: Efficiency analysis
        self.plot_efficiency_analysis()
        
        # Plot 5: Velocity growth (throughput improvement)
        self.plot_velocity_growth()
        
        print(f"Plots saved in {PLOTS_DIR}/")
    
    def plot_individual_scaling(self):
        """Create individual scaling plots for each input size"""
        fig, axes = plt.subplots(2, 4, figsize=(20, 10))
        fig.suptitle('Performance Scaling by Input Size', fontsize=16)
        
        axes = axes.flatten()
        
        for i, input_size in enumerate(INPUT_SIZES):
            ax = axes[i]
            
            if input_size not in self.results:
                ax.set_title(f'{input_size} - No Data')
                ax.set_visible(False)
                continue
            
            threads = []
            times = []
            errors = []
            
            for key, data in self.results[input_size].items():
                if key == 'sequential':
                    thread_count = 1  # For plotting purposes
                else:
                    thread_count = int(key.split('_')[0])
                
                threads.append(thread_count)
                times.append(data['average'])
                errors.append(data['std_dev'])
            
            # Sort by thread count
            sorted_data = sorted(zip(threads, times, errors))
            threads, times, errors = zip(*sorted_data)
            
            ax.errorbar(threads, times, yerr=errors, marker='o', linewidth=2, markersize=6)
            ax.set_title(f'{input_size}')
            ax.set_xlabel('Thread Count')
            ax.set_ylabel('Execution Time (s)')
            ax.set_xscale('log', base=2)
            ax.set_yscale('log')
            ax.grid(True, alpha=0.3)
            ax.set_xticks(threads)
            ax.set_xticklabels([str(t) if t > 1 else 'Seq' for t in threads])
        
        # Hide unused subplot
        if len(INPUT_SIZES) < len(axes):
            axes[-1].set_visible(False)
        
        plt.tight_layout()
        plt.savefig(f'{PLOTS_DIR}/individual_scaling.png', dpi=300, bbox_inches='tight')
        plt.close()
    
    def plot_comparative_scaling(self):
        """Create comparative scaling plot across input sizes"""
        plt.figure(figsize=(12, 8))
        
        colors = plt.cm.tab10(np.linspace(0, 1, len(INPUT_SIZES)))
        
        for i, input_size in enumerate(INPUT_SIZES):
            if input_size not in self.results:
                continue
            
            threads = []
            times = []
            
            for key, data in self.results[input_size].items():
                if key == 'sequential':
                    thread_count = 1
                else:
                    thread_count = int(key.split('_')[0])
                
                threads.append(thread_count)
                times.append(data['average'])
            
            # Sort by thread count
            sorted_data = sorted(zip(threads, times))
            threads, times = zip(*sorted_data)
            
            plt.plot(threads, times, marker='o', linewidth=2, markersize=6, 
                    label=input_size, color=colors[i])
        
        plt.xlabel('Thread Count')
        plt.ylabel('Execution Time (s)')
        plt.title('Performance Scaling Comparison')
        plt.xscale('log', base=2)
        plt.yscale('log')
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        plt.savefig(f'{PLOTS_DIR}/comparative_scaling.png', dpi=300, bbox_inches='tight')
        plt.close()
    
    def plot_speedup_analysis(self):
        """Create speedup analysis plots"""
        plt.figure(figsize=(12, 8))
        
        colors = plt.cm.tab10(np.linspace(0, 1, len(INPUT_SIZES)))
        
        for i, input_size in enumerate(INPUT_SIZES):
            if input_size not in self.results:
                continue
            
            # Get sequential time as baseline
            sequential_time = None
            if 'sequential' in self.results[input_size]:
                sequential_time = self.results[input_size]['sequential']['average']
            elif '1_threads' in self.results[input_size]:
                sequential_time = self.results[input_size]['1_threads']['average']
            
            if sequential_time is None:
                continue
            
            threads = []
            speedups = []
            
            for key, data in self.results[input_size].items():
                if key == 'sequential':
                    continue
                
                thread_count = int(key.split('_')[0])
                speedup = sequential_time / data['average']
                
                threads.append(thread_count)
                speedups.append(speedup)
            
            if not threads:
                continue
            
            # Sort by thread count
            sorted_data = sorted(zip(threads, speedups))
            threads, speedups = zip(*sorted_data)
            
            plt.plot(threads, speedups, marker='o', linewidth=2, markersize=6,
                    label=input_size, color=colors[i])
        
        # Add ideal speedup line
        max_threads = max(THREAD_COUNTS)
        ideal_threads = range(2, max_threads + 1)
        plt.plot(ideal_threads, ideal_threads, '--', color='gray', alpha=0.7, label='Ideal Speedup')
        
        plt.xlabel('Thread Count')
        plt.ylabel('Speedup')
        plt.title('Speedup Analysis')
        plt.xscale('log', base=2)
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        plt.savefig(f'{PLOTS_DIR}/speedup_analysis.png', dpi=300, bbox_inches='tight')
        plt.close()
    
    def plot_efficiency_analysis(self):
        """Create efficiency analysis plots"""
        plt.figure(figsize=(12, 8))
        
        colors = plt.cm.tab10(np.linspace(0, 1, len(INPUT_SIZES)))
        
        for i, input_size in enumerate(INPUT_SIZES):
            if input_size not in self.results:
                continue
            
            # Get sequential time as baseline
            sequential_time = None
            if 'sequential' in self.results[input_size]:
                sequential_time = self.results[input_size]['sequential']['average']
            elif '1_threads' in self.results[input_size]:
                sequential_time = self.results[input_size]['1_threads']['average']
            
            if sequential_time is None:
                continue
            
            threads = []
            efficiencies = []
            
            for key, data in self.results[input_size].items():
                if key == 'sequential':
                    continue
                
                thread_count = int(key.split('_')[0])
                speedup = sequential_time / data['average']
                efficiency = speedup / thread_count
                
                threads.append(thread_count)
                efficiencies.append(efficiency)
            
            if not threads:
                continue
            
            # Sort by thread count
            sorted_data = sorted(zip(threads, efficiencies))
            threads, efficiencies = zip(*sorted_data)
            
            plt.plot(threads, efficiencies, marker='o', linewidth=2, markersize=6,
                    label=input_size, color=colors[i])
        
        # Add ideal efficiency line (100%)
        plt.axhline(y=1.0, linestyle='--', color='gray', alpha=0.7, label='Perfect Efficiency')
        
        plt.xlabel('Thread Count')
        plt.ylabel('Efficiency')
        plt.title('Parallel Efficiency Analysis')
        plt.xscale('log', base=2)
        plt.ylim(0, 1.2)
        plt.legend()
        plt.grid(True, alpha=0.3)
        plt.tight_layout()
        plt.savefig(f'{PLOTS_DIR}/efficiency_analysis.png', dpi=300, bbox_inches='tight')
        plt.close()
    
    def plot_velocity_growth(self):
        """Create velocity growth plot (throughput improvement over sequential)"""
        plt.figure(figsize=(12, 8))
        
        colors = plt.cm.tab10(np.linspace(0, 1, len(INPUT_SIZES)))
        
        for i, input_size in enumerate(INPUT_SIZES):
            if input_size not in self.results:
                continue
            
            # Get sequential time as baseline
            sequential_time = None
            if 'sequential' in self.results[input_size]:
                sequential_time = self.results[input_size]['sequential']['average']
            elif '1_threads' in self.results[input_size]:
                sequential_time = self.results[input_size]['1_threads']['average']
            
            if sequential_time is None:
                continue
            
            threads = []
            velocities = []
            
            # Include sequential as baseline (velocity = 1.0)
            threads.append(1)
            velocities.append(1.0)
            
            for key, data in self.results[input_size].items():
                if key == 'sequential':
                    continue
                
                thread_count = int(key.split('_')[0])
                # Velocity = sequential_time / parallel_time (same as speedup)
                velocity = sequential_time / data['average']
                
                threads.append(thread_count)
                velocities.append(velocity)
            
            if len(threads) <= 1:
                continue
            
            # Sort by thread count
            sorted_data = sorted(zip(threads, velocities))
            threads, velocities = zip(*sorted_data)
            
            plt.plot(threads, velocities, marker='o', linewidth=2, markersize=6,
                    label=input_size, color=colors[i])
        
        # Add ideal velocity growth line
        max_threads = max(THREAD_COUNTS)
        ideal_threads = range(1, max_threads + 1)
        plt.plot(ideal_threads, ideal_threads, '--', color='gray', alpha=0.7, label='Linear Velocity Growth')
        
        plt.xlabel('Thread Count')
        plt.ylabel('Velocity Improvement (x faster)')
        plt.title('Velocity Growth Analysis')
        plt.xscale('log', base=2)
        plt.yscale('log', base=2)
        plt.legend()
        plt.grid(True, alpha=0.3)
        
        # Set proper tick labels
        thread_ticks = [1, 2, 4, 8, 16, 25]
        plt.xticks(thread_ticks, [str(t) for t in thread_ticks])
        velocity_ticks = [1, 2, 4, 8, 16, 25]
        plt.yticks(velocity_ticks, [f'{t}x' for t in velocity_ticks])
        
        plt.tight_layout()
        plt.savefig(f'{PLOTS_DIR}/velocity_growth.png', dpi=300, bbox_inches='tight')
        plt.close()
    
    def print_summary(self):
        """Print benchmark summary"""
        if not self.results:
            print("No results available")
            return
        
        print("\n" + "=" * 80)
        print("BENCHMARK SUMMARY")
        print("=" * 80)
        
        for input_size in INPUT_SIZES:
            if input_size not in self.results:
                continue
            
            print(f"\n{input_size}:")
            print("-" * 40)
            
            # Get sequential time
            sequential_time = None
            if 'sequential' in self.results[input_size]:
                sequential_time = self.results[input_size]['sequential']['average']
                print(f"  Sequential: {sequential_time:.3f}s")
            
            # Show threaded results
            for key, data in sorted(self.results[input_size].items()):
                if key == 'sequential':
                    continue
                
                thread_count = int(key.split('_')[0])
                avg_time = data['average']
                
                speedup_str = ""
                if sequential_time:
                    speedup = sequential_time / avg_time
                    efficiency = speedup / thread_count
                    speedup_str = f" (speedup: {speedup:.2f}x, efficiency: {efficiency:.2f})"
                
                print(f"  {thread_count:2d} threads: {avg_time:.3f}s{speedup_str}")

def main():
    """Main function"""
    print("Ecosystem Simulation Benchmark Tool")
    print("=" * 60)
    
    # Check for required dependencies
    try:
        import matplotlib.pyplot as plt
        import numpy as np
    except ImportError:
        print("Error: Required packages missing. Install with:")
        print("pip install matplotlib numpy")
        sys.exit(1)
    
    benchmark = BenchmarkRunner()
    
    # Check if results already exist
    if os.path.exists(RESULTS_FILE):
        response = input(f"\nResults file {RESULTS_FILE} exists. Load existing results? [y/N]: ")
        if response.lower() in ['y', 'yes']:
            benchmark.load_results()
        else:
            benchmark.run_all_benchmarks()
            benchmark.save_results()
    else:
        benchmark.run_all_benchmarks()
        benchmark.save_results()
    
    # Generate plots and summary
    benchmark.create_performance_plots()
    benchmark.print_summary()
    
    print(f"\nBenchmark complete! Check {PLOTS_DIR}/ for performance plots.")

if __name__ == "__main__":
    main()