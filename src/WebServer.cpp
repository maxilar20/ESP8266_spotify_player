/**
 * @file WebServer.cpp
 * @brief Implementation of Web Server for device selection
 */

#include "WebServer.h"
#include <ESP8266WiFi.h>

// Store HTML/CSS/JS in flash memory to save RAM
const char HTML_HEADER[] PROGMEM = R"EOF(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Spotify NFC Player</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;background:linear-gradient(135deg,#1a1a2e 0%,#16213e 100%);min-height:100vh;color:#fff;padding:20px}
.container{max-width:500px;margin:0 auto}
h1{text-align:center;margin-bottom:10px;color:#1DB954;font-size:1.8em}
.subtitle{text-align:center;color:#888;margin-bottom:30px;font-size:.9em}
.card{background:rgba(255,255,255,.1);border-radius:12px;padding:20px;margin-bottom:20px;backdrop-filter:blur(10px)}
.card h2{font-size:1.1em;margin-bottom:15px;color:#ccc}
.device-select{width:100%;padding:12px;font-size:16px;border:2px solid #333;border-radius:8px;background:#1a1a2e;color:#fff;cursor:pointer}
.device-select:focus{outline:none;border-color:#1DB954}
.btn{width:100%;padding:12px;font-size:16px;font-weight:600;border:none;border-radius:8px;cursor:pointer;margin-top:10px;transition:all .3s}
.btn-primary{background:#1DB954;color:#fff}
.btn-primary:hover{background:#1ed760}
.btn-primary:disabled{background:#555;cursor:not-allowed}
.btn-secondary{background:transparent;border:2px solid #1DB954;color:#1DB954}
.btn-danger{background:#f52;color:#fff}
.btn-danger:hover{background:#e03e3e}
.btn-small{padding:8px;font-size:14px;margin-top:8px}
.info-row{display:flex;justify-content:space-between;padding:8px 0;border-bottom:1px solid rgba(255,255,255,.1);font-size:.9em}
.info-row:last-child{border-bottom:none}
.info-label{color:#888}
.info-value{color:#fff;font-weight:500}
.status-bar{display:flex;align-items:center;gap:10px;padding:10px;border-radius:8px;margin-bottom:10px;font-size:.9em}
.status-bar.success{background:rgba(29,185,84,.2);border:1px solid #1DB954}
.status-bar.error{background:rgba(255,82,82,.2);border:1px solid #f52}
.status-bar.info{background:rgba(100,181,246,.2);border:1px solid #64b5f6}
.status-dot{width:10px;height:10px;border-radius:50%;animation:pulse 2s infinite}
.status-dot.green{background:#1DB954}
.status-dot.red{background:#f52}
.status-dot.blue{background:#64b5f6}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.5}}
.device-info{color:#888;font-size:.85em;margin-top:5px}
.loading{text-align:center;padding:15px;color:#888}
.spinner{border:3px solid #333;border-top:3px solid #1DB954;border-radius:50%;width:30px;height:30px;animation:spin 1s linear infinite;margin:0 auto 10px}
@keyframes spin{0%{transform:rotate(0deg)}100%{transform:rotate(360deg)}}
.toast{position:fixed;bottom:20px;left:50%;transform:translateX(-50%);padding:12px 20px;border-radius:8px;color:#fff;font-weight:500;opacity:0;transition:opacity .3s;z-index:1000}
.toast.show{opacity:1}
.toast.success{background:#1DB954}
.toast.error{background:#f52}
</style>
</head>
<body>
<div class="container">
<h1>🎵 Spotify NFC Player</h1>
<p class="subtitle">Select your playback device</p>
<div class="card">
<div id="statusBar" class="status-bar info">
<div id="statusDot" class="status-dot blue"></div>
<span id="statusText">Checking...</span>
</div>
<div id="currentDevice" class="device-info">Loading...</div>
</div>
<div class="card">
<h2>Select Device</h2>
<div id="loading" class="loading">
<div class="spinner"></div>
<div>Loading devices...</div>
</div>
<div id="deviceSection" style="display:none">
<select id="deviceSelect" class="device-select">
<option value="">Loading...</option>
</select>
<button id="setDeviceBtn" class="btn btn-primary" onclick="setDevice()">Set Device</button>
<button class="btn btn-secondary" onclick="loadDevices()">Refresh</button>
</div>
</div>
<div class="card">
<h2>WiFi Information</h2>
<div id="wifiInfo">
<div class="info-row"><span class="info-label">SSID:</span><span id="wifiSsid" class="info-value">Loading...</span></div>
<div class="info-row"><span class="info-label">IP Address:</span><span id="wifiIp" class="info-value">Loading...</span></div>
<div class="info-row"><span class="info-label">Signal:</span><span id="wifiRssi" class="info-value">Loading...</span></div>
</div>
<button class="btn btn-danger btn-small" onclick="resetWifi()">Reset WiFi Settings</button>
</div>
<div class="card">
<h2>Device Management</h2>
<p style="color:#aaa;font-size:.85em;margin-bottom:10px">Restart the device to apply changes or recover from errors</p>
<button class="btn btn-danger btn-small" onclick="restartDevice()">Restart Device</button>
</div>
<div class="card">
<h2>How to Use</h2>
<p style="color:#aaa;line-height:1.6;font-size:.9em">
1. Make sure Spotify is open on a device<br>
2. Select the device above<br>
3. Tap an NFC tag to play!
</p>
</div>
</div>
<div id="toast" class="toast"></div>
)EOF";

const char HTML_FOOTER[] PROGMEM = R"EOF(
</body>
</html>
)EOF";

const char JAVASCRIPT[] PROGMEM = R"EOF(<script>
let currentDeviceId='';
async function loadDevices(){
const select=document.getElementById('deviceSelect');
const loading=document.getElementById('loading');
const deviceSection=document.getElementById('deviceSection');
loadWifiInfo();
loading.style.display='block';
deviceSection.style.display='none';
try{
const response=await fetch('/api/devices');
const devices=await response.json();
select.innerHTML='<option value="">-- Select a Device --</option>';
devices.forEach(device=>{
const option=document.createElement('option');
option.value=device.id;
option.textContent=device.name+(device.is_active?' (Active)':'');
select.appendChild(option);
});
loading.style.display='none';
deviceSection.style.display='block';
if(devices.length===0){
select.innerHTML='<option value="">No devices found - Open Spotify</option>';
}
}catch(error){
loading.style.display='none';
deviceSection.style.display='block';
showToast('Failed to load devices','error');
}
}
async function loadStatus(){
try{
const response=await fetch('/api/status');
const status=await response.json();
const statusBar=document.getElementById('statusBar');
const statusDot=document.getElementById('statusDot');
const statusText=document.getElementById('statusText');
const currentDevice=document.getElementById('currentDevice');
if(status.authenticated&&status.device_available){
statusBar.className='status-bar success';
statusDot.className='status-dot green';
statusText.textContent='Connected';
currentDevice.textContent='Device: '+status.current_device_name;
currentDeviceId=status.current_device_id;
}else if(status.authenticated){
statusBar.className='status-bar info';
statusDot.className='status-dot blue';
statusText.textContent='Authenticated - No device';
currentDevice.textContent='Select a device below';
}else{
statusBar.className='status-bar error';
statusDot.className='status-dot red';
statusText.textContent='Not connected';
currentDevice.textContent='Check credentials';
}
}catch(error){
console.error('Status error:',error);
}
}
async function setDevice(){
const select=document.getElementById('deviceSelect');
const deviceId=select.value;
const btn=document.getElementById('setDeviceBtn');
if(!deviceId){
showToast('Please select a device','error');
return;
}
btn.disabled=true;
btn.textContent='Setting...';
try{
const response=await fetch('/api/device',{
method:'POST',
headers:{'Content-Type':'application/json'},
body:JSON.stringify({device_id:deviceId})
});
const result=await response.json();
if(response.ok){
showToast('Device set: '+result.device_name,'success');
loadStatus();
}else{
showToast(result.error||'Failed','error');
}
}catch(error){
showToast('Connection error','error');
}
btn.disabled=false;
btn.textContent='Set Device';
}
async function loadWifiInfo(){
try{
const response=await fetch('/api/wifi');
const wifi=await response.json();
document.getElementById('wifiSsid').textContent=wifi.ssid;
document.getElementById('wifiIp').textContent=wifi.ip;
document.getElementById('wifiRssi').textContent=wifi.rssi+' dBm';
}catch(error){
console.error('WiFi info error:',error);
}
}
async function resetWifi(){
if(!confirm('Reset WiFi settings? Device will restart and enter setup mode.'))return;
try{
const response=await fetch('/api/wifi/reset',{method:'POST'});
if(response.ok){
showToast('WiFi reset! Device restarting...','success');
setTimeout(()=>{window.location.reload();},3000);
}else{
showToast('Failed to reset WiFi','error');
}
}catch(error){
showToast('Connection error','error');
}
}
async function restartDevice(){
if(!confirm('Restart the device?'))return;
try{
const response=await fetch('/api/restart',{method:'POST'});
if(response.ok){
showToast('Device restarting...','success');
setTimeout(()=>{window.location.reload();},5000);
}else{
showToast('Failed to restart','error');
}
}catch(error){
showToast('Connection error','error');
}
}
function showToast(message,type){
const toast=document.getElementById('toast');
toast.textContent=message;
toast.className='toast '+type+' show';
setTimeout(()=>{toast.className='toast';},3000);
}
document.addEventListener('DOMContentLoaded',()=>{
loadStatus();
loadDevices();
});
</script>)EOF";

WebServerController::WebServerController(uint16_t port, SpotifyClient& spotifyClient, LedController& ledController)
    : server_(port)
    , spotify_(spotifyClient)
    , leds_(ledController)
{
}

void WebServerController::begin() {
    // Setup route handlers
    server_.on("/", HTTP_GET, [this]() { handleRoot(); });
    server_.on("/api/devices", HTTP_GET, [this]() { handleGetDevices(); });
    server_.on("/api/device", HTTP_POST, [this]() { handleSetDevice(); });
    server_.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    server_.on("/api/wifi", HTTP_GET, [this]() { handleWifiInfo(); });
    server_.on("/api/wifi/reset", HTTP_POST, [this]() { handleWifiReset(); });
    server_.on("/api/restart", HTTP_POST, [this]() { handleRestart(); });
    server_.onNotFound([this]() { handleNotFound(); });

    server_.begin();

    DEBUG_PRINTLN(F("Web server started"));
    DEBUG_PRINT(F("Visit: http://"));
    DEBUG_PRINTLN(WiFi.localIP());
}

void WebServerController::handleClient() {
    server_.handleClient();
}

String WebServerController::getIPAddress() const {
    return WiFi.localIP().toString();
}

void WebServerController::handleRoot() {
    // Send response in chunks using PROGMEM to avoid OOM
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.send(200, "text/html", "");

    server_.sendContent_P(HTML_HEADER);
    server_.sendContent_P(JAVASCRIPT);
    server_.sendContent_P(HTML_FOOTER);

    server_.sendContent(""); // Signal end of content
}

void WebServerController::handleGetDevices() {
    server_.sendHeader("Access-Control-Allow-Origin", "*");

    String json = spotify_.getDevicesJson();
    server_.send(200, "application/json", json);
}

void WebServerController::handleSetDevice() {
    server_.sendHeader("Access-Control-Allow-Origin", "*");

    if (!server_.hasArg("plain")) {
        server_.send(400, "application/json", "{\"error\":\"No body provided\"}");
        return;
    }

    String body = server_.arg("plain");

    // Parse JSON with ArduinoJson
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, body);

    if (error) {
        server_.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        return;
    }

    if (!doc["device_id"].is<String>()) {
        server_.send(400, "application/json", "{\"error\":\"device_id not found\"}");
        return;
    }

    String deviceId = doc["device_id"].as<String>();

    DEBUG_PRINT(F("Setting device to: "));
    DEBUG_PRINTLN(deviceId);

    if (spotify_.setDeviceById(deviceId)) {
        leds_.showDeviceSelected();

        JsonDocument responseDoc;
        responseDoc["success"] = true;
        responseDoc["device_id"] = deviceId;
        responseDoc["device_name"] = spotify_.getDeviceName();

        String response;
        serializeJson(responseDoc, response);
        server_.send(200, "application/json", response);
    } else {
        server_.send(404, "application/json", "{\"error\":\"Device not found or unavailable\"}");
    }
}

void WebServerController::handleStatus() {
    server_.sendHeader("Access-Control-Allow-Origin", "*");

    JsonDocument doc;
    doc["authenticated"] = spotify_.isAuthenticated();
    doc["device_available"] = spotify_.isDeviceAvailable();
    doc["current_device_id"] = spotify_.getDeviceId();
    doc["current_device_name"] = spotify_.getDeviceName();
    doc["ip_address"] = getIPAddress();

    String json;
    serializeJson(doc, json);
    server_.send(200, "application/json", json);
}

void WebServerController::handleNotFound() {
    server_.send(404, "text/plain", "Not Found");
}

void WebServerController::handleWifiInfo() {
    server_.sendHeader("Access-Control-Allow-Origin", "*");

    JsonDocument doc;
    doc["ssid"] = WiFi.SSID();
    doc["ip"] = WiFi.localIP().toString();
    doc["rssi"] = WiFi.RSSI();

    String json;
    serializeJson(doc, json);
    server_.send(200, "application/json", json);
}

void WebServerController::handleWifiReset() {
    server_.sendHeader("Access-Control-Allow-Origin", "*");

    DEBUG_PRINTLN(F("WiFi reset requested via web interface"));

    server_.send(200, "application/json", "{\"success\":true,\"message\":\"WiFi settings will be reset\"}");

    // Give time for response to be sent
    delay(500);

    // Clear WiFi credentials and restart
    WiFi.disconnect(true);
    delay(1000);
    ESP.restart();
}

void WebServerController::handleRestart() {
    server_.sendHeader("Access-Control-Allow-Origin", "*");

    DEBUG_PRINTLN(F("Device restart requested via web interface"));

    server_.send(200, "application/json", "{\"success\":true,\"message\":\"Device restarting\"}");

    // Give time for response to be sent
    delay(500);

    ESP.restart();
}

// No-op methods for API compatibility with main.cpp
void WebServerController::notifyStatusChange() {
    // No-op for synchronous server
}

void WebServerController::notifyNfcTagDetected(const String& /* uri */) {
    // No-op for synchronous server
}

void WebServerController::notifyPlaybackStarted(const String& /* uri */) {
    // No-op for synchronous server
}

void WebServerController::notifyError(const String& /* errorMessage */) {
    // No-op for synchronous server
}
