#include "WebPages.h"
#include <esp_log.h>

static const char *TAG = TAG_WEB_PAGES;

WebPages::WebPages() {
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);
}

String WebPages::generateHomePage(WiFiManager& wifiManager, Storage& storage) {
    String html = getHTMLHeader("EasyWiFi Setup");
    
    html += "<div class='card'>";
    html += "<h1>🌐 EasyWiFi</h1>";
    
    if (wifiManager.isConnected()) {
        html += "<div class='status success'>";
        html += "<div style='font-size:32px;margin-bottom:8px;'>✓</div>";
        html += "<strong>Connected</strong>";
        html += "</div>";
        
        html += "<div style='background:#f8f9fa;padding:16px;border-radius:8px;margin:16px 0;'>";
        html += "<p style='margin:8px 0;'><strong>Network:</strong> " + WiFi.SSID() + "</p>";
        html += "<p style='margin:8px 0;'><strong>IP Address:</strong> " + wifiManager.getLocalIP() + "</p>";
        html += "<p style='margin:8px 0;'><strong>Signal Strength:</strong> " + String(WiFi.RSSI()) + " dBm";
        html += " (" + String(getSignalStrength(WiFi.RSSI())) + ")</p>";
        
        if (wifiManager.getMDNSHostname().length() > 0) {
            html += "<p style='margin:8px 0;'><strong>mDNS:</strong> " + wifiManager.getMDNSHostname() + ".local</p>";
        }
        html += "</div>";
        
        html += "<div class='button-group'>";
        html += "<a href='/scan' class='button'>Change Network</a>";
        html += "<a href='/api/status' class='button'>API Status</a>";
        html += "</div>";
        
        if (storage.isConfigured()) {
            html += "<div class='button-group'>";
            html += "<a href='/reset' class='button danger' onclick='return confirm(\"⚠️ Clear all WiFi credentials and reboot?\\n\\nThis will delete all stored networks and restart in AP mode.\")'>Factory Reset</a>";
            html += "</div>";
        }
        
    } else if (wifiManager.isAPMode()) {
        html += "<div class='status warning'>";
        html += "<div style='font-size:32px;margin-bottom:8px;'>⚙️ <strong>Configuration Mode</strong></div>";
        html += "</div>";
        
        html += "<p style='margin:16px 0;'>Welcome to EasyWiFi! To get started, scan for available networks and configure your WiFi connection.</p>";
        
        html += "<div style='background:#f8f9fa;padding:16px;border-radius:8px;margin:16px 0;'>";
        html += "<p style='margin:8px 0;'><strong>Access Point:</strong> " + wifiManager.getAPSSID() + "</p>";
        html += "<p style='margin:8px 0;'><strong>IP Address:</strong> " + wifiManager.getLocalIP() + "</p>";
        html += "</div>";
        
        html += "<div class='button-group'>";
        html += "<a href='/scan' class='button primary' style='font-size:18px;padding:16px 32px;'>🔍 Scan for Networks</a>";
        html += "</div>";
        
    } else {
        html += "<p class='status'>Status: " + wifiManager.getStatus() + "</p>";
        html += "<div class='button-group'>";
        html += "<a href='/scan' class='button primary'>Scan Networks</a>";
        html += "</div>";
    }
    
    html += "</div>";
    
    // Add info card
    html += "<div class='card' style='margin-top:16px;'>";
    html += "<h2 style='font-size:18px;margin-bottom:12px;'>ℹ️ Information</h2>";
    html += "<p style='font-size:14px;line-height:1.8;'>";
    html += "EasyWiFi provides a simple way to configure WiFi credentials on your ESP32-C3 device. ";
    html += "Scan for networks, enter your password, and the device will remember your settings.";
    html += "</p>";
    html += "</div>";
    
    html += getHTMLFooter();
    return html;
}

String WebPages::generateScanPage(const std::vector<WiFiNetwork>& networks, bool success) {
    String html = getHTMLHeader("Scan Results");
    
    html += "<div class='card'>";
    html += "<h1>📡 Available Networks</h1>";
    
    // Add 2.4GHz notice
    html += "<div style='background:#e3f2fd;border-left:4px solid #2196F3;padding:12px;margin:16px 0;border-radius:4px;'>";
    html += "<p style='margin:0;font-size:14px;'>";
    html += "<strong>ℹ️ Note:</strong> ESP32-C3 only supports 2.4GHz WiFi networks. ";
    html += "5GHz networks are automatically filtered out.";
    html += "</p>";
    html += "</div>";
    
    if (!success) {
        html += "<p class='status error'>✗ Scan failed. Please try again.</p>";
        html += "<div class='button-group'>";
        html += "<a href='/scan' class='button primary'>Scan Again</a>";
        html += "<a href='/' class='button'>Back</a>";
        html += "</div>";
        html += "</div>";
        html += getHTMLFooter();
        return html;
    }
    
    if (networks.empty()) {
        html += "<p class='status warning'>⚠ No networks found</p>";
        html += "<p>Make sure you are in range of a WiFi network.</p>";
        html += "<div class='button-group'>";
        html += "<a href='/scan' class='button primary'>Scan Again</a>";
        html += "<a href='/' class='button'>Back</a>";
        html += "</div>";
        html += "</div>";
        html += getHTMLFooter();
        return html;
    }
    
    html += "<p style='color:#666;margin-bottom:12px;'>Found " + String(networks.size()) + " network(s)</p>";
    
    html += "<div class='network-list'>";
    
    for (const auto& network : networks) {
        html += "<div class='network-item' onclick='selectNetwork(\"" + network.ssid + "\")'>";
        html += "<div class='network-info'>";
        html += "<strong>" + network.ssid + "</strong>";
        
        // Add 2.4GHz badge (all networks shown are 2.4GHz)
        html += " <span style='background:#4CAF50;color:white;padding:2px 6px;border-radius:10px;font-size:10px;font-weight:600;margin-left:6px;'>2.4GHz</span>";
        
        html += "<br>";
        html += "<small>Ch " + String(network.channel) + " | " + network.signalStrength + " (" + String(network.rssi) + " dBm) | " + network.encryptionName + "</small>";
        html += "</div>";
        html += "<div class='network-signal'>";
        html += getSignalBarsHTML(network.signalBars);
        if (network.encryptionType != WIFI_AUTH_OPEN) {
            html += " " + getLockIconHTML();
        }
        html += "</div>";
        html += "<div class='network-action'>";
        html += "<a href='/configure?ssid=" + network.ssid + "' class='button small' onclick='event.stopPropagation()'>Connect</a>";
        html += "</div>";
        html += "</div>";
    }
    
    html += "</div>";
    
    html += "<div class='button-group'>";
    html += "<button onclick='refreshScan()' class='button primary' id='scanBtn'>🔄 Scan Again</button>";
    html += "<a href='/' class='button'>Back to Home</a>";
    html += "</div>";
    
    html += "</div>";
    
    // Add JavaScript for interactive features
    html += "<script>";
    html += "function selectNetwork(ssid) {";
    html += "  window.location.href = '/configure?ssid=' + encodeURIComponent(ssid);";
    html += "}";
    html += "";
    html += "function refreshScan() {";
    html += "  var btn = document.getElementById('scanBtn');";
    html += "  btn.innerHTML = '⏳ Scanning...';";
    html += "  btn.disabled = true;";
    html += "  window.location.href = '/scan';";
    html += "}";
    html += "</script>";
    
    html += getHTMLFooter();
    return html;
}

String WebPages::generateConfigPage(const String& ssid) {
    String html = getHTMLHeader("Configure WiFi");
    
    html += "<div class='card'>";
    html += "<h1>🔐 WiFi Configuration</h1>";
    
    // Add 2.4GHz compatibility warning
    html += "<div style='background:#fff3cd;border-left:4px solid #ffc107;padding:12px;margin:16px 0;border-radius:4px;'>";
    html += "<p style='margin:0;font-size:14px;'>";
    html += "<strong>⚠️ Important:</strong> ESP32-C3 only supports <strong>2.4GHz WiFi networks</strong>. ";
    html += "If your router has separate 2.4GHz and 5GHz networks, make sure to connect to the 2.4GHz one.";
    html += "</p>";
    html += "</div>";
    
    html += "<form method='POST' action='/save' id='configForm' onsubmit='return validateForm()'>";
    
    html += "<div class='form-group'>";
    html += "<label for='ssid'>Network Name (SSID)</label>";
    if (ssid.length() > 0) {
        html += "<input type='text' id='ssid' name='ssid' value='" + ssid + "' required maxlength='32'>";
    } else {
        html += "<input type='text' id='ssid' name='ssid' placeholder='Enter SSID' required maxlength='32'>";
    }
    html += "<small id='ssidError' class='error-text' style='display:none;'>SSID is required</small>";
    html += "</div>";
    
    html += "<div class='form-group'>";
    html += "<label for='password'>Password</label>";
    html += "<input type='password' id='password' name='password' placeholder='Enter password' minlength='8' maxlength='63'>";
    html += "<label style='display:flex;align-items:center;margin-top:8px;'>";
    html += "<input type='checkbox' onclick='togglePassword()' style='width:auto;margin-right:8px;'> Show password";
    html += "</label>";
    html += "<small>Leave empty for open networks (min 8 characters for WPA/WPA2)</small>";
    html += "<small id='passwordError' class='error-text' style='display:none;'>Password must be 8-63 characters</small>";
    html += "</div>";
    
    html += "<div class='button-group'>";
    html += "<button type='submit' class='button primary' id='submitBtn'>Save & Connect</button>";
    html += "<a href='/scan' class='button'>Cancel</a>";
    html += "</div>";
    
    html += "</form>";
    html += "</div>";
    
    // Add JavaScript for form validation and password toggle
    html += "<script>";
    html += "function validateForm() {";
    html += "  var ssid = document.getElementById('ssid').value.trim();";
    html += "  var password = document.getElementById('password').value;";
    html += "  var valid = true;";
    html += "  ";
    html += "  document.getElementById('ssidError').style.display = 'none';";
    html += "  document.getElementById('passwordError').style.display = 'none';";
    html += "  ";
    html += "  if (ssid.length === 0) {";
    html += "    document.getElementById('ssidError').style.display = 'block';";
    html += "    valid = false;";
    html += "  }";
    html += "  ";
    html += "  if (password.length > 0 && password.length < 8) {";
    html += "    document.getElementById('passwordError').style.display = 'block';";
    html += "    valid = false;";
    html += "  }";
    html += "  ";
    html += "  if (valid) {";
    html += "    document.getElementById('submitBtn').innerHTML = 'Saving...';";
    html += "    document.getElementById('submitBtn').disabled = true;";
    html += "  }";
    html += "  ";
    html += "  return valid;";
    html += "}";
    html += "";
    html += "function togglePassword() {";
    html += "  var passwordField = document.getElementById('password');";
    html += "  passwordField.type = passwordField.type === 'password' ? 'text' : 'password';";
    html += "}";
    html += "</script>";
    
    html += getHTMLFooter();
    return html;
}

String WebPages::generateStatusPage(WiFiManager& wifiManager) {
    String html = getHTMLHeader("Connection Status");
    
    html += "<div class='card'>";
    html += "<h1>📶 Connection Status</h1>";
    
    html += "<p>Attempting to connect to WiFi network...</p>";
    html += "<p class='status' id='statusText'>Status: Connecting...</p>";
    
    html += "<div class='loading'></div>";
    
    html += "<p id='message' style='margin-top:20px;color:#666;'></p>";
    
    html += "<div id='actions' class='button-group' style='display:none;'>";
    html += "<a href='/' class='button primary'>View Status</a>";
    html += "<a href='/scan' class='button'>Try Another Network</a>";
    html += "</div>";
    
    html += "<script>";
    html += "var checkCount = 0;";
    html += "var maxChecks = 15;";
    html += "var startTime = Date.now();";
    html += "var maxTime = 35000;";  // 35 seconds max
    html += "";
    html += "function checkStatus() {";
    html += "  checkCount++;";
    html += "  var elapsed = Date.now() - startTime;";
    html += "  ";
    html += "  fetch('/api/status')";
    html += "    .then(r => r.json())";
    html += "    .then(data => {";
    html += "      document.getElementById('statusText').innerHTML = 'Status: ' + data.wifi.status;";
    html += "      ";
    html += "      if (data.wifi.connected) {";
    html += "        document.querySelector('.loading').style.display = 'none';";
    html += "        document.getElementById('statusText').className = 'status success';";
    html += "        document.getElementById('statusText').innerHTML = '✓ Connected to ' + data.wifi.ssid;";
    html += "        document.getElementById('message').innerHTML = 'IP Address: ' + data.wifi.ip + '<br>Redirecting to home...';";
    html += "        setTimeout(() => window.location.href = '/', 2000);";
    html += "      } else if (data.wifi.ap_mode && checkCount > 3) {";
    html += "        document.querySelector('.loading').style.display = 'none';";
    html += "        document.getElementById('statusText').className = 'status error';";
    html += "        document.getElementById('statusText').innerHTML = '✗ Connection Failed';";
    html += "        document.getElementById('message').innerHTML = 'Could not connect to the network.<br><br>';";
    html += "        document.getElementById('message').innerHTML += '<strong>Possible reasons:</strong><br>';";
    html += "        document.getElementById('message').innerHTML += '• Incorrect password<br>';";
    html += "        document.getElementById('message').innerHTML += '• Network out of range<br>';";
    html += "        document.getElementById('message').innerHTML += '• Network uses 5GHz (ESP32-C3 only supports 2.4GHz)<br>';";
    html += "        document.getElementById('message').innerHTML += '• Router security settings';";
    html += "        document.getElementById('actions').style.display = 'block';";
    html += "      } else if (checkCount >= maxChecks || elapsed >= maxTime) {";
    html += "        document.querySelector('.loading').style.display = 'none';";
    html += "        document.getElementById('statusText').className = 'status error';";
    html += "        document.getElementById('statusText').innerHTML = '⚠ Connection Timeout';";
    html += "        document.getElementById('message').innerHTML = 'Connection is taking longer than expected.<br>The device may still be trying to connect.';";
    html += "        document.getElementById('actions').style.display = 'block';";
    html += "      } else {";
    html += "        var secondsElapsed = Math.floor(elapsed / 1000);";
    html += "        document.getElementById('message').innerHTML = 'Connecting... (' + secondsElapsed + 's elapsed)';";
    html += "        setTimeout(checkStatus, 2000);";
    html += "      }";
    html += "    })";
    html += "    .catch(err => {";
    html += "      if (checkCount < maxChecks && elapsed < maxTime) {";
    html += "        setTimeout(checkStatus, 2000);";
    html += "      } else {";
    html += "        document.querySelector('.loading').style.display = 'none';";
    html += "        document.getElementById('statusText').className = 'status error';";
    html += "        document.getElementById('statusText').innerHTML = '✗ Error';";
    html += "        document.getElementById('message').innerHTML = 'Lost connection to device.';";
    html += "        document.getElementById('actions').style.display = 'block';";
    html += "      }";
    html += "    });";
    html += "}";
    html += "";
    html += "setTimeout(checkStatus, 3000);";
    html += "</script>";
    
    html += "</div>";
    html += getHTMLFooter();
    return html;
}

String WebPages::generateSuccessPage() {
    String html = getHTMLHeader("Success");
    
    html += "<div class='card'>";
    html += "<h1>✓ Success</h1>";
    html += "<p class='status success'>Connected to WiFi successfully!</p>";
    html += "<a href='/' class='button primary'>Go Home</a>";
    html += "</div>";
    
    html += getHTMLFooter();
    return html;
}

String WebPages::generateErrorPage(const String& errorMessage) {
    String html = getHTMLHeader("Error");
    
    html += "<div class='card'>";
    html += "<h1>✗ Error</h1>";
    html += "<p class='status error'>" + errorMessage + "</p>";
    html += "<a href='/' class='button'>Go Home</a>";
    html += "</div>";
    
    html += getHTMLFooter();
    return html;
}

String WebPages::generateResetPage() {
    String html = getHTMLHeader("Reset Complete");
    
    html += "<div class='card'>";
    html += "<h1>🔄 Reset Complete</h1>";
    html += "<p class='status success'>All WiFi credentials have been cleared.</p>";
    html += "<p>Device will reboot in 2 seconds...</p>";
    html += "</div>";
    
    html += getHTMLFooter();
    return html;
}

// ==================== HTML Components ====================

String WebPages::getHTMLHeader(const String& title) {
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>" + title + "</title>";
    html += getCSS();
    html += "</head><body>";
    return html;
}

String WebPages::getHTMLFooter() {
    String html = "<div class='footer'>";
    html += "<small>EasyWiFi v0.1 | ESP32-C3</small>";
    html += "</div>";
    html += "</body></html>";
    return html;
}

String WebPages::getCSS() {
    String css = "<style>";
    css += "* { margin: 0; padding: 0; box-sizing: border-box; }";
    css += "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; background: #f5f5f5; padding: 20px; }";
    css += ".card { background: white; border-radius: 12px; padding: 24px; margin: 0 auto; max-width: 600px; box-shadow: 0 2px 8px rgba(0,0,0,0.1); }";
    css += "h1 { font-size: 24px; margin-bottom: 16px; color: #333; }";
    css += "p { margin: 12px 0; line-height: 1.6; color: #666; }";
    css += ".status { padding: 12px; border-radius: 8px; font-weight: 500; }";
    css += ".status.success { background: #d4edda; color: #155724; }";
    css += ".status.warning { background: #fff3cd; color: #856404; }";
    css += ".status.error { background: #f8d7da; color: #721c24; }";
    css += ".button { display: inline-block; padding: 12px 24px; margin: 8px 4px; border: none; border-radius: 8px; text-decoration: none; font-weight: 500; cursor: pointer; transition: all 0.2s; font-size: 16px; }";
    css += ".button.primary { background: #007bff; color: white; }";
    css += ".button.primary:hover:not(:disabled) { background: #0056b3; }";
    css += ".button.primary:disabled { background: #6c9bd4; cursor: not-allowed; }";
    css += ".button.danger { background: #dc3545; color: white; }";
    css += ".button.danger:hover { background: #c82333; }";
    css += ".button.small { padding: 8px 16px; font-size: 14px; }";
    css += ".button:not(.primary):not(.danger) { background: #6c757d; color: white; }";
    css += ".button:not(.primary):not(.danger):hover { background: #545b62; }";
    css += ".button-group { margin-top: 20px; text-align: center; }";
    css += ".network-list { margin: 16px 0; max-height: 400px; overflow-y: auto; }";
    css += ".network-item { display: flex; align-items: center; padding: 12px; margin: 8px 0; border: 1px solid #ddd; border-radius: 8px; background: #fafafa; transition: all 0.2s; }";
    css += ".network-item:hover { background: #f0f0f0; border-color: #bbb; }";
    css += ".network-info { flex: 1; }";
    css += ".network-signal { margin: 0 12px; white-space: nowrap; }";
    css += ".network-action { }";
    css += ".form-group { margin: 16px 0; }";
    css += "label { display: block; margin-bottom: 8px; font-weight: 500; color: #333; }";
    css += "input[type='text'], input[type='password'] { width: 100%; padding: 12px; border: 1px solid #ddd; border-radius: 8px; font-size: 16px; }";
    css += "input:focus { outline: none; border-color: #007bff; box-shadow: 0 0 0 3px rgba(0,123,255,0.1); }";
    css += "input.error { border-color: #dc3545; }";
    css += "small { display: block; margin-top: 4px; color: #999; font-size: 14px; }";
    css += ".error-text { color: #dc3545; font-weight: 500; }";
    css += ".footer { text-align: center; margin-top: 32px; color: #999; font-size: 14px; }";
    css += ".loading { border: 4px solid #f3f3f3; border-top: 4px solid #007bff; border-radius: 50%; width: 40px; height: 40px; animation: spin 1s linear infinite; margin: 20px auto; }";
    css += "@keyframes spin { 0% { transform: rotate(0deg); } 100% { transform: rotate(360deg); } }";
    css += "@media (max-width: 640px) { body { padding: 12px; } .card { padding: 16px; } h1 { font-size: 20px; } .button { padding: 10px 20px; font-size: 14px; } }";
    css += "</style>";
    return css;
}

String WebPages::getSignalBarsHTML(int bars) {
    String html = "<span style='font-size: 18px;'>";
    for (int i = 0; i < 4; i++) {
        if (i < bars) {
            html += "📶";
        } else {
            html += "▫";
        }
    }
    html += "</span>";
    return html;
}

String WebPages::getLockIconHTML() {
    return "<span>🔒</span>";
}

