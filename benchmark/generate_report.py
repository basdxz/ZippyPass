#!/usr/bin/env python3
"""
Generates a consolidated benchmark report in Markdown format from individual benchmark results
"""
import json
import sys
import os
import glob
from datetime import datetime

def main():
    if len(sys.argv) != 2:
        print("Usage: generate_report.py <output_markdown_file>")
        sys.exit(1)
    
    output_file = sys.argv[1]
    
    # Find all JSON result files in the benchmark directories
    benchmark_dir = os.path.join(os.path.dirname(output_file), "benchmark")
    result_files = glob.glob(f"{benchmark_dir}/*/results.json")
    
    if not result_files:
        print("No benchmark results found.")
        sys.exit(1)
    
    # Load all benchmark results
    benchmarks = []
    for result_file in result_files:
        with open(result_file, 'r') as f:
            try:
                data = json.load(f)
                benchmarks.append(data)
            except json.JSONDecodeError:
                print(f"Error parsing {result_file}. Skipping.")
    
    # Sort benchmarks by name
    benchmarks.sort(key=lambda x: x["benchmark"])
    
    # Generate the markdown report
    with open(output_file, 'w') as f:
        f.write("# Struct Field Reordering Optimization Benchmark Results\n\n")
        f.write(f"Generated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n\n")
        
        # Summary table
        f.write("## Summary\n\n")
        f.write("| Benchmark | Original (ms) | Optimized (ms) | Improvement |\n")
        f.write("|-----------|--------------|---------------|-------------|\n")
        
        total_improvement = 0
        improved_count = 0
        
        for benchmark in benchmarks:
            name = benchmark["benchmark"]
            orig_avg = benchmark["original"]["avg"]
            opt_avg = benchmark["optimized"]["avg"]
            improvement = benchmark["improvement_percent"]
            
            f.write(f"| {name} | {orig_avg:.2f} | {opt_avg:.2f} | {improvement:.2f}% |\n")
            
            if float(improvement) > 0:
                total_improvement += float(improvement)
                improved_count += 1
        
        # Add average improvement for benchmarks that showed improvement
        if improved_count > 0:
            avg_improvement = total_improvement / improved_count
            f.write(f"\nAverage improvement across {improved_count} benchmarks: **{avg_improvement:.2f}%**\n")
        
        # Detailed results
        f.write("\n## Detailed Results\n\n")
        
        for benchmark in benchmarks:
            name = benchmark["benchmark"]
            f.write(f"### {name}\n\n")
            
            orig = benchmark["original"]
            opt = benchmark["optimized"]
            improvement = benchmark["improvement_percent"]
            
            f.write(f"- **Original**: avg={orig['avg']:.2f}ms, min={orig['min']:.2f}ms, max={orig['max']:.2f}ms (iterations={orig['iterations']})\n")
            f.write(f"- **Optimized**: avg={opt['avg']:.2f}ms, min={opt['min']:.2f}ms, max={opt['max']:.2f}ms (iterations={opt['iterations']})\n")
            f.write(f"- **Improvement**: {improvement:.2f}%\n\n")
    
    print(f"Benchmark report generated: {output_file}")

if __name__ == "__main__":
    main()
