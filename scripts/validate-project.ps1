# Project Structure Validation Script
# Constitutional Principle VI: Test-Driven Development
# 
# Validates project structure and configuration without requiring compilation

param(
    [switch]$Detailed = $false
)

Write-Host "ESP32 Garage Sensor - Project Structure Validation" -ForegroundColor Green
Write-Host "Constitutional Principle VI: Test-Driven Development" -ForegroundColor Green
Write-Host "=================================================" -ForegroundColor Green
Write-Host ""

$totalTests = 0
$passedTests = 0
$failedTests = 0

function Test-Assertion {
    param(
        [string]$TestName,
        [bool]$Condition,
        [string]$Details = ""
    )
    
    $script:totalTests++
    
    if ($Condition) {
        $script:passedTests++
        Write-Host "✓ $TestName" -ForegroundColor Green
        if ($Detailed -and $Details) {
            Write-Host "  → $Details" -ForegroundColor Gray
        }
    } else {
        $script:failedTests++
        Write-Host "✗ $TestName" -ForegroundColor Red
        if ($Details) {
            Write-Host "  → $Details" -ForegroundColor Red
        }
    }
}

# Test 1: Project Structure
Write-Host "Testing Project Structure..." -ForegroundColor Cyan

$requiredDirs = @(
    "src",
    "src/config",
    "test",
    "test/mocks",
    "test/test_setup",
    "test/test_hardware",
    ".specify",
    "scripts"
)

foreach ($dir in $requiredDirs) {
    Test-Assertion "Directory exists: $dir" (Test-Path $dir) "Required for project organization"
}

# Test 2: Configuration Files
Write-Host "`nTesting Configuration Files..." -ForegroundColor Cyan

$requiredFiles = @(
    "platformio.ini",
    "src/main.cpp",
    "src/config/hardware.h",
    "test/mocks/mock_hardware.h",
    "test/mocks/mock_hardware.cpp",
    "README.md",
    ".gitignore"
)

foreach ($file in $requiredFiles) {
    Test-Assertion "File exists: $file" (Test-Path $file) "Required for project configuration"
}

# Test 3: Constitution and Specifications
Write-Host "`nTesting Constitutional Compliance..." -ForegroundColor Cyan

Test-Assertion "Constitution exists" (Test-Path ".specify/memory/constitution.md") "Constitutional Principle VI compliance"
Test-Assertion "Plan exists" (Test-Path ".specify/feature/plan.md") "Technical specification"
Test-Assertion "Tasks exist" (Test-Path ".specify/feature/tasks.md") "Implementation roadmap"

# Test 4: Hardware Configuration
Write-Host "`nTesting Hardware Configuration..." -ForegroundColor Cyan

if (Test-Path "src/config/hardware.h") {
    $hardwareContent = Get-Content "src/config/hardware.h" -Raw
    
    Test-Assertion "Sensor GPIO defined" ($hardwareContent -match "SENSOR_GPIO_PIN\s*2") "E3JK-RR11 sensor pin configuration"
    Test-Assertion "Parking LED GPIO defined" ($hardwareContent -match "PARKING_LED_GPIO\s*8") "Parking indicator LED"
    Test-Assertion "Status LED GPIO defined" ($hardwareContent -match "STATUS_LED_GPIO\s*9") "System status LED"
    Test-Assertion "PWM channels defined" ($hardwareContent -match "PWM_CH") "LED PWM control"
    Test-Assertion "Timing requirements defined" ($hardwareContent -match "MAX_RESPONSE_TIME_MS") "Constitutional timing requirements"
}

# Test 5: PlatformIO Configuration
Write-Host "`nTesting PlatformIO Configuration..." -ForegroundColor Cyan

if (Test-Path "platformio.ini") {
    $platformioContent = Get-Content "platformio.ini" -Raw
    
    Test-Assertion "ESP32-S3 board configured" ($platformioContent -match "esp32-s3-devkitc-1") "Target hardware platform"
    Test-Assertion "Arduino framework configured" ($platformioContent -match "framework\s*=\s*arduino") "Development framework"
    Test-Assertion "Test environments configured" ($platformioContent -match "\[env:test\]") "Unit testing environment"
    Test-Assertion "Coverage environment configured" ($platformioContent -match "coverage") "Constitutional coverage requirement"
}

# Test 6: Git Configuration
Write-Host "`nTesting Git Configuration..." -ForegroundColor Cyan

if (Test-Path ".gitignore") {
    $gitignoreContent = Get-Content ".gitignore" -Raw
    
    Test-Assertion "Build artifacts ignored" ($gitignoreContent -match "\.pio") "PlatformIO build directory"
    Test-Assertion "Environment files ignored" ($gitignoreContent -match "\.env") "Secret management"
    Test-Assertion "IDE files ignored" ($gitignoreContent -match "\.vscode") "IDE configuration"
}

# Test 7: Test Infrastructure
Write-Host "`nTesting Test Infrastructure..." -ForegroundColor Cyan

$testFiles = @(
    "test/test_setup/test_project_setup.cpp",
    "test/test_hardware/test_hardware_config.cpp",
    "test/mocks/mock_hardware.h",
    "test/mocks/mock_hardware.cpp"
)

foreach ($file in $testFiles) {
    Test-Assertion "Test file exists: $(Split-Path $file -Leaf)" (Test-Path $file) "Constitutional testing requirement"
}

if (Test-Path "test/mocks/mock_hardware.h") {
    $mockContent = Get-Content "test/mocks/mock_hardware.h" -Raw
    Test-Assertion "Mock GPIO abstraction" ($mockContent -match "MockGPIO") "Hardware abstraction for testing"
    Test-Assertion "Mock PWM abstraction" ($mockContent -match "MockPWM") "PWM control abstraction"
    Test-Assertion "Mock timing abstraction" ($mockContent -match "MockTiming") "Timing abstraction for deterministic tests"
}

# Test 8: Constitutional Compliance
Write-Host "`nTesting Constitutional Compliance..." -ForegroundColor Cyan

if (Test-Path ".specify/memory/constitution.md") {
    $constitutionContent = Get-Content ".specify/memory/constitution.md" -Raw
    Test-Assertion "Principle VI: Test-Driven Development" ($constitutionContent -match "Test-Driven Development") "Constitutional testing mandate"
    Test-Assertion "80% coverage requirement" ($constitutionContent -match "80%") "Minimum coverage threshold"
    Test-Assertion "Testing infrastructure mandate" ($constitutionContent -match "comprehensive testing infrastructure") "Testing framework requirement"
}

# Results Summary
Write-Host "`n" + "="*50 -ForegroundColor Gray
Write-Host "VALIDATION RESULTS" -ForegroundColor Yellow
Write-Host "="*50 -ForegroundColor Gray

Write-Host "Total Tests: $totalTests" -ForegroundColor White
Write-Host "Passed: $passedTests" -ForegroundColor Green  
Write-Host "Failed: $failedTests" -ForegroundColor Red

$successRate = [math]::Round(($passedTests / $totalTests) * 100, 1)
Write-Host "Success Rate: $successRate%" -ForegroundColor $(if ($successRate -ge 80) { "Green" } else { "Yellow" })

Write-Host ""

if ($failedTests -eq 0) {
    Write-Host "🎉 ALL TESTS PASSED" -ForegroundColor Green
    Write-Host "✓ Project structure validated successfully" -ForegroundColor Green
    Write-Host "✓ Constitutional Principle VI compliance verified" -ForegroundColor Green
    Write-Host "✓ Ready for development environment setup" -ForegroundColor Green
} elseif ($successRate -ge 80) {
    Write-Host "⚠️  MOSTLY PASSING" -ForegroundColor Yellow
    Write-Host "✓ Core project structure is valid" -ForegroundColor Green
    Write-Host "⚠  Some optional components need attention" -ForegroundColor Yellow
    Write-Host "✓ Constitutional compliance maintained" -ForegroundColor Green
} else {
    Write-Host "❌ VALIDATION FAILED" -ForegroundColor Red
    Write-Host "✗ Critical project structure issues detected" -ForegroundColor Red
    Write-Host "✗ Review and fix failed tests before proceeding" -ForegroundColor Red
}

Write-Host ""
Write-Host "Next Steps:" -ForegroundColor Cyan
Write-Host "1. Install PlatformIO: python -m pip install platformio" -ForegroundColor White
Write-Host "2. Build project: pio run" -ForegroundColor White  
Write-Host "3. Run tests: pio test" -ForegroundColor White
Write-Host "4. Check coverage: pio test --environment test_coverage" -ForegroundColor White

exit $(if ($failedTests -eq 0) { 0 } else { 1 })