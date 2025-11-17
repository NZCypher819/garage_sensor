# Native Test Build Script
# Constitutional Principle VI: Test-Driven Development
# 
# Compiles and runs basic project validation tests without PlatformIO

# Determine compiler based on platform
if (Get-Command "g++" -ErrorAction SilentlyContinue) {
    $compiler = "g++"
} elseif (Get-Command "cl" -ErrorAction SilentlyContinue) {
    $compiler = "cl"
} else {
    Write-Host "ERROR: No C++ compiler found (g++ or cl)" -ForegroundColor Red
    Write-Host "Please install:" -ForegroundColor Yellow
    Write-Host "  - MinGW-w64 (for g++)" -ForegroundColor Yellow
    Write-Host "  - Visual Studio Build Tools (for cl)" -ForegroundColor Yellow
    Write-Host "  - Or install PlatformIO which includes toolchain" -ForegroundColor Yellow
    exit 1
}

Write-Host "ESP32 Garage Sensor - Native Test Build" -ForegroundColor Green
Write-Host "Constitutional Principle VI: Test-Driven Development" -ForegroundColor Green
Write-Host "=============================================" -ForegroundColor Green
Write-Host ""

$testFile = "test\native_test_runner.cpp"
$outputFile = "test\native_test_runner.exe"

if (-not (Test-Path $testFile)) {
    Write-Host "ERROR: Test file not found: $testFile" -ForegroundColor Red
    exit 1
}

Write-Host "Compiling native tests with $compiler..." -ForegroundColor Cyan

try {
    if ($compiler -eq "g++") {
        & g++ -std=c++17 -Wall -Wextra -O2 -o $outputFile $testFile
    } else {
        & cl /EHsc /std:c++17 /Fe:$outputFile $testFile
    }
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Compilation failed" -ForegroundColor Red
        exit 1
    }
    
    Write-Host "Compilation successful!" -ForegroundColor Green
    Write-Host ""
    
    Write-Host "Running native tests..." -ForegroundColor Cyan
    & $outputFile
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host ""
        Write-Host "✓ All tests passed - Project validation successful!" -ForegroundColor Green
        Write-Host "✓ Constitutional Principle VI compliance verified" -ForegroundColor Green
    } else {
        Write-Host ""
        Write-Host "✗ Some tests failed - Review required" -ForegroundColor Red
        exit 1
    }
    
} catch {
    Write-Host "ERROR: Build process failed: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
} finally {
    # Clean up executable
    if (Test-Path $outputFile) {
        Remove-Item $outputFile -Force
    }
}

Write-Host ""
Write-Host "Native test validation complete!" -ForegroundColor Green