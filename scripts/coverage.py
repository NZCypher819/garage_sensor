import os
import subprocess
import sys

def run_coverage():
    """Generate test coverage report using gcov and lcov"""
    
    print("=== Garage Sensor Test Coverage Report ===")
    print("Constitutional Requirement VI: 80% minimum coverage")
    
    # Run tests with coverage
    result = subprocess.run(['pio', 'test', '-e', 'test_coverage'], 
                          capture_output=True, text=True)
    
    if result.returncode != 0:
        print(f"Test execution failed: {result.stderr}")
        return False
    
    # Generate coverage data
    build_dir = ".pio/build/test_coverage"
    if not os.path.exists(build_dir):
        print(f"Build directory not found: {build_dir}")
        return False
    
    try:
        # Capture coverage data
        subprocess.run(['lcov', '--capture', '--directory', build_dir, 
                       '--output-file', 'coverage.info'], check=True)
        
        # Filter out external libraries and test files
        subprocess.run(['lcov', '--remove', 'coverage.info', 
                       '*/test/*', '*/lib/*', '*/Unity/*',
                       '--output-file', 'coverage_filtered.info'], check=True)
        
        # Generate HTML report
        subprocess.run(['genhtml', 'coverage_filtered.info', 
                       '--output-directory', 'coverage'], check=True)
        
        # Parse coverage percentage
        result = subprocess.run(['lcov', '--summary', 'coverage_filtered.info'],
                              capture_output=True, text=True, check=True)
        
        lines = result.stdout.split('\n')
        for line in lines:
            if 'lines' in line and '%' in line:
                coverage_pct = line.split('%')[0].split()[-1]
                print(f"Line Coverage: {coverage_pct}%")
                
                # Constitutional validation
                try:
                    coverage_val = float(coverage_pct)
                    if coverage_val >= 80.0:
                        print("✅ CONSTITUTIONAL COMPLIANCE: Coverage exceeds 80% minimum")
                        return True
                    else:
                        print("❌ CONSTITUTIONAL VIOLATION: Coverage below 80% minimum")
                        print(f"Required: 80%, Actual: {coverage_val}%")
                        return False
                except ValueError:
                    print(f"Could not parse coverage percentage: {coverage_pct}")
                    return False
        
        print("Coverage report generated in coverage/ directory")
        return True
        
    except subprocess.CalledProcessError as e:
        print(f"Coverage generation failed: {e}")
        return False
    except FileNotFoundError:
        print("lcov not found - install with: apt-get install lcov")
        return False

if __name__ == "__main__":
    success = run_coverage()
    sys.exit(0 if success else 1)