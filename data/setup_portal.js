/**
 * WiFi Setup Portal JavaScript
 * Handles form validation, submission, and real-time status updates
 */

class WiFiSetupPortal {
    constructor() {
        this.form = document.getElementById('wifiForm');
        this.submitBtn = document.getElementById('submit-btn');
        this.submitText = document.getElementById('submit-text');
        this.submitLoading = document.getElementById('submit-loading');
        this.progressSection = document.getElementById('progress-section');
        this.progressText = document.getElementById('progress-text');
        this.progressBar = document.getElementById('progress-bar');
        
        this.statusCheckInterval = null;
        this.isSubmitting = false;
        this.validationTimeout = null;
        
        this.init();
    }
    
    init() {
        this.bindEvents();
        this.setupValidation();
        this.startStatusCheck();
        
        console.log('WiFi Setup Portal initialized');
    }
    
    bindEvents() {
        // Form submission
        this.form.addEventListener('submit', this.handleSubmit.bind(this));
        
        // Real-time validation
        const inputs = this.form.querySelectorAll('input');
        inputs.forEach(input => {
            input.addEventListener('input', this.handleInputChange.bind(this));
            input.addEventListener('blur', this.validateField.bind(this));
        });
        
        // Password confirmation
        const confirmPassword = document.getElementById('confirm_password');
        confirmPassword.addEventListener('input', this.validatePasswordMatch.bind(this));
        
        // Prevent form resubmission
        window.addEventListener('beforeunload', this.handleBeforeUnload.bind(this));
    }
    
    setupValidation() {
        // SSID validation rules
        const ssidInput = document.getElementById('ssid');
        ssidInput.addEventListener('input', (e) => {
            const value = e.target.value;
            const help = document.getElementById('ssid-help');
            
            if (value.length === 0) {
                help.textContent = 'Network name is required';
                help.style.color = '#e53e3e';
            } else if (value.length > 32) {
                help.textContent = 'Network name too long (max 32 characters)';
                help.style.color = '#e53e3e';
            } else if (!/^[\x20-\x7E]*$/.test(value)) {
                help.textContent = 'Only printable ASCII characters allowed';
                help.style.color = '#e53e3e';
            } else {
                help.textContent = `${value.length}/32 characters`;
                help.style.color = '#718096';
            }
        });
        
        // Password validation rules
        const passwordInput = document.getElementById('password');
        passwordInput.addEventListener('input', (e) => {
            const value = e.target.value;
            const help = document.getElementById('password-help');
            
            if (value.length === 0) {
                help.textContent = 'Password is required';
                help.style.color = '#e53e3e';
            } else if (value.length < 8) {
                help.textContent = 'Password should be at least 8 characters';
                help.style.color = '#dd6b20';
            } else if (value.length > 63) {
                help.textContent = 'Password too long (max 63 characters)';
                help.style.color = '#e53e3e';
            } else {
                help.textContent = `Strong password (${value.length}/63 characters)`;
                help.style.color = '#38a169';
            }
        });
    }
    
    handleInputChange(e) {
        const field = e.target;
        
        // Clear validation timeout
        if (this.validationTimeout) {
            clearTimeout(this.validationTimeout);
        }
        
        // Debounced validation
        this.validationTimeout = setTimeout(() => {
            this.validateField({ target: field });
        }, 300);
    }
    
    validateField(e) {
        const field = e.target;
        const value = field.value.trim();
        
        // Remove existing validation classes
        field.classList.remove('valid', 'invalid');
        
        // Field-specific validation
        switch (field.id) {
            case 'ssid':
                if (value.length > 0 && value.length <= 32 && /^[\x20-\x7E]*$/.test(value)) {
                    field.classList.add('valid');
                } else if (value.length > 0) {
                    field.classList.add('invalid');
                }
                break;
                
            case 'password':
                if (value.length >= 8 && value.length <= 63) {
                    field.classList.add('valid');
                } else if (value.length > 0) {
                    field.classList.add('invalid');
                }
                break;
                
            case 'confirm_password':
                const password = document.getElementById('password').value;
                if (value === password && value.length > 0) {
                    field.classList.add('valid');
                } else if (value.length > 0) {
                    field.classList.add('invalid');
                }
                break;
        }
        
        this.updateSubmitButton();
    }
    
    validatePasswordMatch() {
        const password = document.getElementById('password').value;
        const confirmPassword = document.getElementById('confirm_password').value;
        const help = document.getElementById('confirm-help');
        
        if (confirmPassword.length === 0) {
            help.textContent = 'Please confirm your password';
            help.style.color = '#718096';
        } else if (password !== confirmPassword) {
            help.textContent = 'Passwords do not match';
            help.style.color = '#e53e3e';
        } else {
            help.textContent = 'Passwords match';
            help.style.color = '#38a169';
        }
        
        this.updateSubmitButton();
    }
    
    updateSubmitButton() {
        const ssid = document.getElementById('ssid').value.trim();
        const password = document.getElementById('password').value;
        const confirmPassword = document.getElementById('confirm_password').value;
        
        const isValid = 
            ssid.length > 0 && ssid.length <= 32 &&
            password.length >= 8 && password.length <= 63 &&
            password === confirmPassword &&
            !this.isSubmitting;
        
        this.submitBtn.disabled = !isValid;
    }
    
    async handleSubmit(e) {
        e.preventDefault();
        
        if (this.isSubmitting) {
            return;
        }
        
        this.isSubmitting = true;
        this.showSubmittingState();
        
        try {
            const formData = new FormData(this.form);
            const credentials = {
                ssid: formData.get('ssid').trim(),
                password: formData.get('password')
            };
            
            // Validate one more time
            if (!this.validateCredentials(credentials)) {
                throw new Error('Invalid credentials');
            }
            
            // Submit credentials
            const response = await fetch('/configure', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                },
                body: JSON.stringify(credentials)
            });
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            
            const result = await response.json();
            
            if (result.status === 'validating') {
                this.showValidatingState();
                this.startConnectionMonitoring();
            } else {
                throw new Error(result.message || 'Configuration failed');
            }
            
        } catch (error) {
            console.error('Submit error:', error);
            this.showError('Failed to submit credentials: ' + error.message);
            this.resetSubmitButton();
        }
    }
    
    validateCredentials(credentials) {
        // SSID validation
        if (!credentials.ssid || credentials.ssid.length === 0) {
            this.showError('WiFi network name is required');
            return false;
        }
        
        if (credentials.ssid.length > 32) {
            this.showError('WiFi network name too long (max 32 characters)');
            return false;
        }
        
        if (!/^[\x20-\x7E]*$/.test(credentials.ssid)) {
            this.showError('WiFi network name contains invalid characters');
            return false;
        }
        
        // Password validation
        if (!credentials.password || credentials.password.length === 0) {
            this.showError('WiFi password is required');
            return false;
        }
        
        if (credentials.password.length > 63) {
            this.showError('WiFi password too long (max 63 characters)');
            return false;
        }
        
        return true;
    }
    
    showSubmittingState() {
        this.submitText.textContent = 'Submitting...';
        this.submitLoading.classList.remove('hidden');
        this.submitBtn.disabled = true;
        this.form.classList.add('submitting');
        
        this.hideAllMessages();
    }
    
    showValidatingState() {
        this.hideForm();
        this.progressSection.classList.remove('hidden');
        this.progressText.textContent = 'Testing WiFi connection...';
        this.updateProgress(25);
    }
    
    startConnectionMonitoring() {
        let attempts = 0;
        const maxAttempts = 30; // 30 seconds timeout
        
        const checkStatus = async () => {
            try {
                const response = await fetch('/status');
                const status = await response.json();
                
                attempts++;
                const progress = Math.min(25 + (attempts * 2.5), 90);
                this.updateProgress(progress);
                
                switch (status.state) {
                    case 'connected':
                        this.updateProgress(100);
                        this.showSuccess('WiFi connected successfully! Your garage sensor is now online.');
                        this.showCompletionMessage();
                        return;
                        
                    case 'failed':
                        this.showError('Connection failed. Please check your credentials and try again.');
                        this.showForm();
                        this.resetSubmitButton();
                        return;
                        
                    case 'connecting':
                        this.progressText.textContent = 'Connecting to WiFi network...';
                        break;
                        
                    case 'validating':
                        this.progressText.textContent = 'Validating credentials...';
                        break;
                }
                
                if (attempts < maxAttempts) {
                    setTimeout(checkStatus, 1000);
                } else {
                    this.showError('Connection timeout. Please try again.');
                    this.showForm();
                    this.resetSubmitButton();
                }
                
            } catch (error) {
                console.error('Status check error:', error);
                this.showError('Unable to check connection status. Please try again.');
                this.showForm();
                this.resetSubmitButton();
            }
        };
        
        setTimeout(checkStatus, 1000);
    }
    
    resetSubmitButton() {
        this.isSubmitting = false;
        this.submitText.textContent = 'Connect to WiFi';
        this.submitLoading.classList.add('hidden');
        this.form.classList.remove('submitting');
        this.progressSection.classList.add('hidden');
        this.updateSubmitButton();
    }
    
    updateProgress(percentage) {
        this.progressBar.style.width = percentage + '%';
    }
    
    hideForm() {
        this.form.style.display = 'none';
    }
    
    showForm() {
        this.form.style.display = 'block';
        this.progressSection.classList.add('hidden');
    }
    
    showCompletionMessage() {
        setTimeout(() => {
            const container = document.querySelector('.setup-container');
            container.innerHTML = `
                <div class="header">
                    <div class="device-icon">✅</div>
                    <h1>Setup Complete!</h1>
                    <p class="subtitle">Your garage sensor is connected to WiFi</p>
                </div>
                <div class="success">
                    <strong>Success!</strong> Your device is now connected to your WiFi network and ready to use.
                </div>
                <div class="security-notice">
                    <p>🔒 Your WiFi credentials are encrypted and stored securely.</p>
                    <p>The setup portal will close automatically.</p>
                </div>
            `;
        }, 2000);
    }
    
    showMessage(type, message) {
        this.hideAllMessages();
        
        const messageEl = document.getElementById(type + '-message');
        if (messageEl) {
            messageEl.textContent = message;
            messageEl.classList.remove('hidden');
        }
    }
    
    showError(message) {
        this.showMessage('error', message);
    }
    
    showSuccess(message) {
        this.showMessage('success', message);
    }
    
    showInfo(message) {
        this.showMessage('info', message);
    }
    
    hideAllMessages() {
        ['info-message', 'error-message', 'success-message'].forEach(id => {
            const el = document.getElementById(id);
            if (el) el.classList.add('hidden');
        });
    }
    
    startStatusCheck() {
        // Check initial status
        this.checkDeviceStatus();
        
        // Set up periodic status checking
        this.statusCheckInterval = setInterval(() => {
            this.checkDeviceStatus();
        }, 5000);
    }
    
    async checkDeviceStatus() {
        try {
            const response = await fetch('/status');
            const status = await response.json();
            
            // If already connected, show success message
            if (status.state === 'connected') {
                this.showSuccess('Device is already connected to WiFi.');
                this.hideForm();
            }
            
        } catch (error) {
            console.error('Device status check failed:', error);
        }
    }
    
    handleBeforeUnload(e) {
        if (this.isSubmitting) {
            e.preventDefault();
            e.returnValue = 'WiFi setup is in progress. Are you sure you want to leave?';
            return e.returnValue;
        }
    }
    
    // Public API for testing
    simulateSuccess() {
        this.showSuccess('WiFi connected successfully! (Simulated for testing)');
        this.showCompletionMessage();
    }
    
    simulateError(message = 'Connection failed (simulated for testing)') {
        this.showError(message);
        this.resetSubmitButton();
    }
}

// Initialize when DOM is loaded
document.addEventListener('DOMContentLoaded', function() {
    window.wifiSetup = new WiFiSetupPortal();
    
    // Expose for testing/debugging
    if (window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1') {
        window.testWifiSetup = window.wifiSetup;
        console.log('Test methods available: testWifiSetup.simulateSuccess(), testWifiSetup.simulateError()');
    }
});

// Service worker for offline functionality (optional enhancement)
if ('serviceWorker' in navigator && window.location.protocol === 'https:') {
    window.addEventListener('load', function() {
        navigator.serviceWorker.register('/sw.js')
            .then(registration => console.log('ServiceWorker registered'))
            .catch(error => console.log('ServiceWorker registration failed'));
    });
}