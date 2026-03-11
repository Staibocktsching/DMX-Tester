#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

// ====== KONFIGURATION ======
#define SSID_AP "DMX-Tester"
#define PASSWORD_AP "12345678"
#define DMX_CHANNELS 512
#define BAUD_RATE 250000

// RS485 Pins (richtig für Wemos D1 mini)
#define RS485_DE D8    // GPIO15 - Data Enable

// EEPROM Adressen
#define EEPROM_SIZE 4096
#define PRESET_ADDR 0
#define PRESET_SIZE (DMX_CHANNELS + 2)
#define COLOR_ADDR 3500  // Farb-Speicher (am Ende)

// IP ADRESSE HIER ÄNDERN (optional):
IPAddress apIP(192, 168, 4, 1);
IPAddress gateway(192, 168, 4, 1);
IPAddress subnet(255, 255, 255, 0);

// ====== GLOBALE VARIABLEN ======
ESP8266WebServer server(80);
uint8_t dmxData[DMX_CHANNELS];
uint32_t dmxTimer = 0;
const uint32_t DMX_INTERVAL = 25;
uint8_t presets[10][DMX_CHANNELS];
bool presetLoaded[10] = {false};
String currentColor = "#ffb300";  // Standard-Farbe

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== DMX Tester START ===");
  
  EEPROM.begin(EEPROM_SIZE);
  
  pinMode(RS485_DE, OUTPUT);
  digitalWrite(RS485_DE, LOW);
  
  memset(dmxData, 0, DMX_CHANNELS);
  loadAllPresets();
  loadColorFromEEPROM();  // Farbe laden beim Start
  
  setupWiFiAP();
  
  server.on("/", handleRoot);
  server.on("/api/dmx", handleDMXGet);
  server.on("/api/dmx", HTTP_POST, handleDMXSet);
  server.on("/api/preset", handlePreset);
  server.on("/api/send", handleSendDMX);
  server.on("/api/color", handleColor);  // Neue Color-Route
  
  server.begin();
  Serial.println("Web Server gestartet");
  
  Serial1.begin(BAUD_RATE);
}

// ====== LOOP ======
void loop() {
  server.handleClient();
  
  if (millis() - dmxTimer >= DMX_INTERVAL) {
    dmxTimer = millis();
    sendDMXFrame();
  }
}

// ====== WiFi AP Setup ======
void setupWiFiAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, gateway, subnet);
  WiFi.softAP(SSID_AP, PASSWORD_AP);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(IP);
  Serial.print("Connect to: ");
  Serial.println(SSID_AP);
}

// ====== DMX FRAME SENDEN ======
void sendDMXFrame() {
  digitalWrite(RS485_DE, HIGH);
  Serial1.flush();
  Serial1.write(0);
  
  for (int i = 0; i < DMX_CHANNELS; i++) {
    Serial1.write(dmxData[i]);
  }
  
  Serial1.flush();
  digitalWrite(RS485_DE, LOW);
}

// ====== WEB HANDLER - ROOT (HTML) ======
void handleRoot() {
  String html = "";
  html += "<!DOCTYPE html><html><head>";
  html += "<meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>DMX Tester</title>";
  html += "<style>";
  html += "* { margin: 0; padding: 0; box-sizing: border-box; }";
  html += "body { font-family: 'Courier New', monospace; background: #0a0a0a; color: #ffb300; min-height: 100vh; padding: 10px; }";
  html += ".container { max-width: 1000px; margin: 0 auto; background: #1a1a1a; border: 3px solid #ffb300; border-radius: 8px; padding: 20px; box-shadow: 0 0 30px rgba(255, 179, 0, 0.6), 0 0 60px rgba(255, 179, 0, 0.4), inset 0 0 30px rgba(255, 179, 0, 0.1), inset 0 0 60px rgba(255, 179, 0, 0.05); }";
  html += "h1 { text-align: center; color: #ffb300; margin-bottom: 15px; font-size: 32px; font-weight: bold; letter-spacing: 3px; text-shadow: 0 0 10px rgba(255, 179, 0, 0.8), 0 0 20px rgba(255, 179, 0, 0.6), 0 0 30px rgba(255, 179, 0, 0.4), 0 0 40px rgba(255, 179, 0, 0.2); }";
  html += ".toggle-section { display: flex; justify-content: center; margin-bottom: 20px; gap: 10px; }";
  html += ".toggle-btn { background: #333; color: #ffb300; border: 1px solid #ffb300; padding: 8px 15px; border-radius: 3px; font-weight: bold; cursor: pointer; font-size: 11px; text-transform: uppercase; letter-spacing: 1px; transition: all 0.2s; }";
  html += ".toggle-btn.active { background: #ffb300; color: #000; box-shadow: 0 0 15px rgba(255, 179, 0, 0.5); }";
  html += ".toggle-btn:hover { background: #ffb300; color: #000; }";
  html += ".page { display: none; }";
  html += ".page.show { display: block; }";
  html += ".display-area { background: #0f0f0f; border: 2px solid #ffb300; border-radius: 5px; padding: 20px; margin-bottom: 25px; box-shadow: inset 0 0 20px rgba(255, 179, 0, 0.1); }";
  html += ".display-title { color: #ffb300; font-size: 12px; margin-bottom: 10px; text-transform: uppercase; letter-spacing: 2px; }";
  html += ".channels-display { display: grid; grid-template-columns: repeat(auto-fit, minmax(45px, 1fr)); gap: 8px; margin-bottom: 15px; }";
  html += ".channel-display { background: #000; border: 1px solid #333; border-radius: 3px; padding: 8px; text-align: center; font-size: 10px; }";
  html += ".channel-display.active { border-color: #ffb300; box-shadow: 0 0 8px rgba(255, 179, 0, 0.4); }";
  html += ".channel-display-num { color: #666; font-size: 9px; margin-bottom: 3px; }";
  html += ".channel-display-val { color: #ffb300; font-weight: bold; font-size: 12px; }";
  html += ".input-section { background: #1a1a1a; border: 2px solid #ffb300; border-radius: 5px; padding: 20px; box-shadow: inset 0 0 20px rgba(255, 179, 0, 0.1); }";
  html += ".input-title { color: #ffb300; font-size: 12px; margin-bottom: 15px; text-transform: uppercase; letter-spacing: 2px; }";
  html += ".input-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-bottom: 15px; }";
  html += ".input-group { display: flex; flex-direction: column; }";
  html += ".input-label { color: #ffb300; font-size: 11px; margin-bottom: 5px; text-transform: uppercase; letter-spacing: 1px; }";
  html += "input[type=\"number\"] { background: #0f0f0f; border: 1px solid #ffb300; color: #ffb300; padding: 10px; border-radius: 3px; font-family: 'Courier New', monospace; font-size: 14px; }";
  html += "input[type=\"number\"]:focus { outline: none; box-shadow: 0 0 10px rgba(255, 179, 0, 0.5); }";
  html += "input[type=\"text\"] { background: #0f0f0f; border: 1px solid #ffb300; color: #ffb300; padding: 10px; border-radius: 3px; font-family: monospace; font-size: 14px; }";
  html += "input[type=\"text\"]:focus { outline: none; box-shadow: 0 0 10px rgba(255, 179, 0, 0.5); }";
  html += ".button-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 10px; }";
  html += "button { background: #ffb300; color: #000; border: none; padding: 12px; border-radius: 3px; font-weight: bold; cursor: pointer; font-size: 12px; text-transform: uppercase; letter-spacing: 1px; transition: all 0.2s; }";
  html += "button:hover { background: #ffc933; box-shadow: 0 0 15px rgba(255, 179, 0, 0.5); }";
  html += "button:active { transform: scale(0.98); }";
  html += ".presets-section { margin-top: 25px; border-top: 1px dashed #333; padding-top: 15px; }";
  html += ".presets-title { color: #ffb300; font-size: 12px; margin-bottom: 10px; text-transform: uppercase; letter-spacing: 2px; }";
  html += ".preset-buttons { display: grid; grid-template-columns: repeat(5, 1fr); gap: 8px; }";
  html += ".preset-btn { background: #333; color: #ffb300; border: 1px solid #ffb300; padding: 8px; border-radius: 3px; font-weight: bold; cursor: pointer; font-size: 11px; transition: all 0.2s; }";
  html += ".preset-btn:hover { background: #ffb300; color: #000; }";
  html += ".status-bar { background: #0f0f0f; color: #ffb300; padding: 10px; border-radius: 3px; margin-top: 10px; font-size: 11px; text-align: center; border: 1px solid #ffb300; }";
  html += ".colors-grid { display: grid; grid-template-columns: repeat(5, 1fr); gap: 15px; margin-bottom: 20px; }";
  html += ".color-preset { background: #333 !important; color: #ffb300 !important; border: 2px solid #ffb300 !important; padding: 15px !important; border-radius: 5px !important; font-weight: bold !important; cursor: pointer !important; font-size: 11px !important; text-transform: uppercase !important; letter-spacing: 1px !important; transition: all 0.3s !important; display: flex !important; flex-direction: column !important; align-items: center !important; gap: 8px !important; }";
  html += ".color-preset:hover { background: #ffb300 !important; color: #000 !important; box-shadow: 0 0 20px rgba(255, 179, 0, 0.6) !important; }";
  html += ".color-preset:active { transform: scale(0.95) !important; }";
  html += ".color-box { width: 30px; height: 30px; border-radius: 3px; border: 2px solid #fff; display: inline-block; }";
  html += "</style></head><body>";
  html += "<div class=\"container\">";
  html += "<h1>DMX TESTER</h1>";
  html += "<div class=\"toggle-section\">";
  html += "<button class=\"toggle-btn active\" onclick=\"showPage('control')\">CONTROL</button>";
  html += "<button class=\"toggle-btn\" onclick=\"showPage('display')\">MONITOR</button>";
  html += "<button class=\"toggle-btn\" onclick=\"showPage('colors')\">COLORS</button>";
  html += "</div>";
  html += "<div id=\"control\" class=\"page show\">";
  html += "<div class=\"input-section\">";
  html += "<div class=\"input-title\">Channel Control</div>";
  html += "<div class=\"input-grid\">";
  html += "<div class=\"input-group\"><label class=\"input-label\">Channel (1-512)</label><input type=\"number\" id=\"channelInput\" min=\"1\" max=\"512\" value=\"1\"></div>";
  html += "<div class=\"input-group\"><label class=\"input-label\">Value (0-255)</label><input type=\"number\" id=\"valueInput\" min=\"0\" max=\"255\" value=\"0\"></div>";
  html += "</div>";
  html += "<div class=\"button-grid\">";
  html += "<button onclick=\"setChannelManual()\">SET</button>";
  html += "<button onclick=\"incrementChannel()\">+1</button>";
  html += "<button onclick=\"decrementChannel()\">-1</button>";
  html += "<button onclick=\"allOn()\">ALL ON</button>";
  html += "<button onclick=\"allOff()\">ALL OFF</button>";
  html += "<button onclick=\"setAllValue(128)\">50%</button>";
  html += "<button onclick=\"chase()\">CHASE</button>";
  html += "<button onclick=\"resetAll()\">RESET</button>";
  html += "</div>";
  html += "<div class=\"presets-section\">";
  html += "<div class=\"presets-title\">Scenes</div>";
  html += "<div class=\"preset-buttons\" id=\"presetsArea\"></div>";
  html += "</div>";
  html += "<div class=\"status-bar\" id=\"statusBar\">READY</div>";
  html += "</div>";
  html += "</div>";
  html += "<div id=\"display\" class=\"page\">";
  html += "<div class=\"display-area\">";
  html += "<div class=\"display-title\">Channel Display (All 512)</div>";
  html += "<div class=\"channels-display\" id=\"displayArea\"></div>";
  html += "</div>";
  html += "</div>";
  html += "<div id=\"colors\" class=\"page\">";
  html += "<div class=\"input-section\">";
  html += "<div class=\"input-title\">UI Color Theme</div>";
  html += "<div class=\"colors-grid\">";
  html += "<button class=\"color-preset\" onclick=\"setColor('#ffb300', 'AMBER')\"><div class=\"color-box\" style=\"background:#ffb300;\"></div>AMBER</button>";
  html += "<button class=\"color-preset\" onclick=\"setColor('#00ff00', 'GREEN')\"><div class=\"color-box\" style=\"background:#00ff00;\"></div>GREEN</button>";
  html += "<button class=\"color-preset\" onclick=\"setColor('#512DA8', 'VIOLETT')\"><div class=\"color-box\" style=\"background:#512DA8;\"></div>VIOLETT</button>";
  html += "<button class=\"color-preset\" onclick=\"setColor('#00ffff', 'CYAN')\"><div class=\"color-box\" style=\"background:#00ffff;\"></div>CYAN</button>";
  html += "<button class=\"color-preset\" onclick=\"setColor('#ff6600', 'ORANGE')\"><div class=\"color-box\" style=\"background:#ff6600;\"></div>ORANGE</button>";

  html += "</div>";
  html += "<div style=\"margin-top:20px;padding:15px;background:#0f0f0f;border:1px solid #ffb300;border-radius:3px;\">";
  html += "<div class=\"input-label\">Custom Color (HEX)</div>";
  html += "<div style=\"display:flex;gap:10px;margin-top:10px;\">";
  html += "<input type=\"text\" id=\"customColor\" placeholder=\"#ffb300\" maxlength=\"7\">";
  html += "<button onclick=\"applyCustomColor()\" style=\"padding:8px 15px;\">APPLY</button>";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"status-bar\" id=\"colorStatus\">Current: AMBER</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "</div>";
  html += "<script>";
  html += "const DMX_CHANNELS = 512; let dmxValues = new Array(DMX_CHANNELS).fill(0); let chaseActive = false;";
  html += "function showPage(page) {";
  html += "document.querySelectorAll('.page').forEach(p => p.classList.remove('show'));";
  html += "document.getElementById(page).classList.add('show');";
  html += "document.querySelectorAll('.toggle-btn').forEach(b => b.classList.remove('active'));";
  html += "event.target.classList.add('active');";
  html += "}";
  html += "function initDisplay() {";
  html += "const d = document.getElementById('displayArea');";
  html += "for (let i = 0; i < DMX_CHANNELS; i++) {";
  html += "const c = document.createElement('div');";
  html += "c.className = 'channel-display';";
  html += "c.id = 'ch' + i;";
  html += "c.innerHTML = '<div class=\"channel-display-num\">' + (i+1) + '</div><div class=\"channel-display-val\" id=\"val' + i + '\">0</div>';";
  html += "d.appendChild(c);";
  html += "}";
  html += "}";
  html += "function initPresets() {";
  html += "const p = document.getElementById('presetsArea');";
  html += "for (let i = 0; i < 10; i++) {";
  html += "const b = document.createElement('button');";
  html += "b.className = 'preset-btn';";
  html += "b.textContent = 'S' + (i+1);";
  html += "b.onclick = (function(n) { return function() { loadPreset(n); } })(i);";
  html += "p.appendChild(b);";
  html += "}";
  html += "}";
  html += "function updateDisplay() {";
  html += "for (let i = 0; i < DMX_CHANNELS; i++) {";
  html += "const v = dmxValues[i] || 0;";
  html += "const elem = document.getElementById('val' + i);";
  html += "if (elem) elem.textContent = v;";
  html += "const ch = document.getElementById('ch' + i);";
  html += "if (v > 0) ch.classList.add('active');";
  html += "else ch.classList.remove('active');";
  html += "}";
  html += "}";
  html += "function setChannelValue(ch, val) {";
  html += "ch = parseInt(ch) - 1;";
  html += "val = Math.max(0, Math.min(255, parseInt(val)));";
  html += "if (ch >= 0 && ch < DMX_CHANNELS) {";
  html += "dmxValues[ch] = val;";
  html += "updateDisplay();";
  html += "sendDMX();";
  html += "document.getElementById('channelInput').value = parseInt(ch) + 1;";
  html += "document.getElementById('valueInput').value = val;";
  html += "}";
  html += "}";
  html += "function setChannelManual() { const ch = document.getElementById('channelInput').value; const val = document.getElementById('valueInput').value; setChannelValue(ch, val); updateStatus('CH ' + ch + ' = ' + val); }";
  html += "function incrementChannel() { const val = Math.min(255, parseInt(document.getElementById('valueInput').value) + 1); document.getElementById('valueInput').value = val; setChannelManual(); }";
  html += "function decrementChannel() { const val = Math.max(0, parseInt(document.getElementById('valueInput').value) - 1); document.getElementById('valueInput').value = val; setChannelManual(); }";
  html += "function allOn() { for (let i = 0; i < DMX_CHANNELS; i++) dmxValues[i] = 255; updateDisplay(); sendDMX(); updateStatus('ALL CHANNELS = 255'); }";
  html += "function allOff() { for (let i = 0; i < DMX_CHANNELS; i++) dmxValues[i] = 0; updateDisplay(); sendDMX(); updateStatus('ALL CHANNELS = 0'); }";
  html += "function setAllValue(v) { for (let i = 0; i < DMX_CHANNELS; i++) dmxValues[i] = v; updateDisplay(); sendDMX(); updateStatus('ALL CHANNELS = ' + v); }";
  html += "function resetAll() { for (let i = 0; i < DMX_CHANNELS; i++) dmxValues[i] = 0; updateDisplay(); sendDMX(); document.getElementById('channelInput').value = 1; document.getElementById('valueInput').value = 0; updateStatus('RESET - ALL OFF'); }";
  html += "function chase() { chaseActive = !chaseActive; if (chaseActive) { let ch = 0; const chaseLoop = setInterval(() => { if (!chaseActive) { clearInterval(chaseLoop); return; } dmxValues[ch] = 0; ch = (ch + 1) % DMX_CHANNELS; dmxValues[ch] = 255; updateDisplay(); sendDMX(); }, 50); updateStatus('CHASE RUNNING...'); } else { updateStatus('CHASE STOPPED'); } }";
  html += "function sendDMX() { fetch('/api/dmx', {method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({values: dmxValues})}); }";
  html += "function loadDMXState() { fetch('/api/dmx').then(r => r.json()).then(d => { dmxValues = d.values; updateDisplay(); }); }";
  html += "function loadPreset(n) { fetch('/api/preset?preset=' + n + '&action=load').then(r => r.json()).then(d => { dmxValues = d.values; updateDisplay(); sendDMX(); updateStatus('SCENE ' + (n+1) + ' LOADED'); }); }";
  html += "function updateStatus(msg) { document.getElementById('statusBar').textContent = msg; }";
  html += "function hexToRgb(hex) { const result = /^#?([a-f\\d]{2})([a-f\\d]{2})([a-f\\d]{2})$/i.exec(hex); return result ? {r: parseInt(result[1], 16), g: parseInt(result[2], 16), b: parseInt(result[3], 16)} : {r: 255, g: 179, b: 0}; }";
  html += "let currentColor = '#ffb300';";
  html += "function setColor(hexColor, colorName) { currentColor = hexColor; applyColorToUI(); document.getElementById('colorStatus').textContent = 'Current: ' + colorName; document.getElementById('customColor').value = hexColor; saveColorToServer(hexColor); }";
  html += "function applyCustomColor() { const input = document.getElementById('customColor').value.trim(); if (/^#[0-9A-F]{6}$/i.test(input)) { setColor(input, 'CUSTOM'); } else { alert('Invalid HEX color! Use format: #RRGGBB'); } }";
  html += "function saveColorToServer(color) { fetch('/api/color', {method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({color: color})}); }";
  html += "function applyColorToUI() { const rgb = hexToRgb(currentColor); const rgbStr = rgb.r + ',' + rgb.g + ',' + rgb.b; document.body.style.color = currentColor; document.querySelectorAll('.container').forEach(el => { el.style.borderColor = currentColor; el.style.boxShadow = '0 0 30px rgba(' + rgbStr + ', 0.6), 0 0 60px rgba(' + rgbStr + ', 0.4), inset 0 0 30px rgba(' + rgbStr + ', 0.1), inset 0 0 60px rgba(' + rgbStr + ', 0.05)'; }); document.querySelectorAll('h1').forEach(el => { el.style.color = currentColor; el.style.textShadow = '0 0 10px rgba(' + rgbStr + ', 0.8), 0 0 20px rgba(' + rgbStr + ', 0.6), 0 0 30px rgba(' + rgbStr + ', 0.4), 0 0 40px rgba(' + rgbStr + ', 0.2)'; }); document.querySelectorAll('.toggle-btn').forEach(el => { el.style.borderColor = currentColor; el.style.color = currentColor; }); document.querySelectorAll('.toggle-btn.active').forEach(el => { el.style.backgroundColor = currentColor; el.style.color = '#000'; }); document.querySelectorAll('.display-title, .input-title, .input-label, .presets-title, .channel-display-val').forEach(el => { el.style.color = currentColor; }); document.querySelectorAll('.input-section, .display-area').forEach(el => { el.style.borderColor = currentColor; el.style.boxShadow = 'inset 0 0 20px rgba(' + rgbStr + ', 0.15)'; }); document.querySelectorAll('.status-bar, #statusBar, #colorStatus').forEach(el => { el.style.color = currentColor; el.style.borderColor = currentColor; }); document.querySelectorAll('input[type=\"number\"], input[type=\"text\"]').forEach(el => { el.style.borderColor = currentColor; el.style.color = currentColor; }); document.querySelectorAll('.preset-btn, .color-preset').forEach(el => { el.style.borderColor = currentColor; el.style.color = currentColor; }); }";
  html += "document.addEventListener('DOMContentLoaded', function() { initDisplay(); initPresets(); loadDMXState(); setInterval(loadDMXState, 500); document.getElementById('channelInput').addEventListener('keypress', function(e) { if (e.key === 'Enter') setChannelManual(); }); document.getElementById('valueInput').addEventListener('keypress', function(e) { if (e.key === 'Enter') setChannelManual(); }); });";
  html += "</script></body></html>";
  
  server.send(200, "text/html; charset=utf-8", html);
}

// ====== WEB HANDLER - DMX GET ======
void handleDMXGet() {
  String json = "{\"values\":[";
  for (int i = 0; i < DMX_CHANNELS; i++) {
    json += String(dmxData[i]);
    if (i < DMX_CHANNELS - 1) json += ",";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

// ====== WEB HANDLER - DMX POST ======
void handleDMXSet() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
    return;
  }
  
  String body = server.arg("plain");
  int startIdx = body.indexOf("[");
  int endIdx = body.lastIndexOf("]");
  
  if (startIdx == -1 || endIdx == -1) {
    server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
    return;
  }
  
  String arrayStr = body.substring(startIdx + 1, endIdx);
  int lastPos = 0;
  
  for (int i = 0; i < DMX_CHANNELS; i++) {
    int nextComma = arrayStr.indexOf(",", lastPos);
    if (nextComma == -1) nextComma = arrayStr.length();
    
    String valStr = arrayStr.substring(lastPos, nextComma);
    int val = valStr.toInt();
    dmxData[i] = constrain(val, 0, 255);
    lastPos = nextComma + 1;
  }
  
  server.send(200, "application/json", "{\"status\":\"OK\"}");
}

// ====== WEB HANDLER - PRESETS ======
void handlePreset() {
  String action = server.arg("action");
  int presetNum = server.arg("preset").toInt();
  
  if (presetNum < 0 || presetNum > 9) {
    server.send(400, "application/json", "{\"error\":\"Invalid preset\"}");
    return;
  }
  
  if (action == "save") {
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"error\":\"No data\"}");
      return;
    }
    
    String body = server.arg("plain");
    int startIdx = body.indexOf("[");
    int endIdx = body.lastIndexOf("]");
    
    if (startIdx == -1 || endIdx == -1) {
      server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
      return;
    }
    
    String arrayStr = body.substring(startIdx + 1, endIdx);
    int lastPos = 0;
    
    for (int i = 0; i < DMX_CHANNELS; i++) {
      int nextComma = arrayStr.indexOf(",", lastPos);
      if (nextComma == -1) nextComma = arrayStr.length();
      
      String valStr = arrayStr.substring(lastPos, nextComma);
      int val = valStr.toInt();
      presets[presetNum][i] = constrain(val, 0, 255);
      lastPos = nextComma + 1;
    }
    
    savePresetToEEPROM(presetNum);
    presetLoaded[presetNum] = true;
    server.send(200, "application/json", "{\"status\":\"Preset saved\"}");
    
  } else if (action == "load") {
    String json = "{\"values\":[";
    for (int i = 0; i < DMX_CHANNELS; i++) {
      json += String(presets[presetNum][i]);
      if (i < DMX_CHANNELS - 1) json += ",";
    }
    json += "]}";
    server.send(200, "application/json", json);
  }
}

void handleSendDMX() {
  server.send(200, "application/json", "{\"status\":\"Sending\"}");
}

// ====== WEB HANDLER - COLOR ======
void handleColor() {
  if (server.method() == HTTP_POST) {
    // Farbe speichern
    if (server.hasArg("plain")) {
      String body = server.arg("plain");
      int colorIdx = body.indexOf("color");
      if (colorIdx != -1) {
        int startIdx = body.indexOf("\"", colorIdx) + 1;
        int endIdx = body.indexOf("\"", startIdx);
        String color = body.substring(startIdx, endIdx);
        
        currentColor = color;
        saveColorToEEPROM(color);
        server.send(200, "application/json", "{\"status\":\"Color saved\"}");
      }
    }
  } else if (server.method() == HTTP_GET) {
    // Aktuelle Farbe auslesen
    String json = "{\"color\":\"" + currentColor + "\"}";
    server.send(200, "application/json", json);
  }
}

void savePresetToEEPROM(uint8_t presetNum) {
  int addr = PRESET_ADDR + (presetNum * PRESET_SIZE);
  for (int i = 0; i < DMX_CHANNELS; i++) {
    EEPROM.write(addr + i, presets[presetNum][i]);
  }
  uint16_t checksum = 0;
  for (int i = 0; i < DMX_CHANNELS; i++) {
    checksum += presets[presetNum][i];
  }
  EEPROM.write(addr + DMX_CHANNELS, checksum & 0xFF);
  EEPROM.write(addr + DMX_CHANNELS + 1, (checksum >> 8) & 0xFF);
  EEPROM.commit();
  Serial.printf("Preset %d saved\n", presetNum);
}

void loadPresetFromEEPROM(uint8_t presetNum) {
  int addr = PRESET_ADDR + (presetNum * PRESET_SIZE);
  for (int i = 0; i < DMX_CHANNELS; i++) {
    presets[presetNum][i] = EEPROM.read(addr + i);
  }
  presetLoaded[presetNum] = true;
  Serial.printf("Preset %d loaded\n", presetNum);
}

void loadAllPresets() {
  for (int i = 0; i < 10; i++) {
    loadPresetFromEEPROM(i);
  }
}

// ====== FARB-EEPROM FUNKTIONEN ======
void saveColorToEEPROM(String color) {
  // Farbe als 6 Hex-Zeichen speichern (#RRGGBB)
  // Bytes: [0]=R, [1]=G, [2]=B
  if (color.length() >= 7 && color[0] == '#') {
    String r = color.substring(1, 3);
    String g = color.substring(3, 5);
    String b = color.substring(5, 7);
    
    uint8_t rVal = (uint8_t)strtol(r.c_str(), NULL, 16);
    uint8_t gVal = (uint8_t)strtol(g.c_str(), NULL, 16);
    uint8_t bVal = (uint8_t)strtol(b.c_str(), NULL, 16);
    
    EEPROM.write(COLOR_ADDR, rVal);
    EEPROM.write(COLOR_ADDR + 1, gVal);
    EEPROM.write(COLOR_ADDR + 2, bVal);
    EEPROM.write(COLOR_ADDR + 3, 0xFF);  // Checksumme: "gültig"
    EEPROM.commit();
    
    Serial.print("Color saved: ");
    Serial.println(color);
  }
}

void loadColorFromEEPROM() {
  // Prüfe ob gültige Farbe im EEPROM gespeichert ist
  uint8_t checksum = EEPROM.read(COLOR_ADDR + 3);
  
  if (checksum == 0xFF) {
    uint8_t r = EEPROM.read(COLOR_ADDR);
    uint8_t g = EEPROM.read(COLOR_ADDR + 1);
    uint8_t b = EEPROM.read(COLOR_ADDR + 2);
    
    // Zurück zu HEX konvertieren
    char hexColor[8];
    sprintf(hexColor, "#%02X%02X%02X", r, g, b);
    currentColor = String(hexColor);
    
    Serial.print("Color loaded: ");
    Serial.println(currentColor);
  } else {
    // Standard-Farbe wenn nichts gespeichert
    currentColor = "#ffb300";
    Serial.println("No color in EEPROM, using default (AMBER)");
  }
}
