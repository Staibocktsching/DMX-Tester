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
#define PRESET_SIZE (DMX_CHANNELS + 2) // +2 für Checksumme

// ====== GLOBALE VARIABLEN ======
ESP8266WebServer server(80);
uint8_t dmxData[DMX_CHANNELS];
uint32_t dmxTimer = 0;
const uint32_t DMX_INTERVAL = 25; // ~40fps
uint8_t presets[10][DMX_CHANNELS]; // 10 Szenen-Presets speichern
bool presetLoaded[10] = {false};

// ====== SETUP ======
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== DMX Tester START ===");
  
  // EEPROM initialisieren
  EEPROM.begin(EEPROM_SIZE);
  
  // RS485 Pin setup
  pinMode(RS485_DE, OUTPUT);
  digitalWrite(RS485_DE, LOW); // Empfangsmode starten
  
  // DMX Daten initialisieren
  memset(dmxData, 0, DMX_CHANNELS);
  loadAllPresets();
  
  // WiFi AP starten
  setupWiFiAP();
  
  // Web Server Routes
  server.on("/", handleRoot);
  server.on("/api/dmx", handleDMXGet);
  server.on("/api/dmx", HTTP_POST, handleDMXSet);
  server.on("/api/preset", handlePreset);
  server.on("/api/send", handleSendDMX);
  
  server.begin();
  Serial.println("Web Server gestartet");
  
  // Serial1 für DMX starten (Hardware UART für TX auf GPIO1)
  Serial1.begin(BAUD_RATE);
}

// ====== LOOP ======
void loop() {
  server.handleClient();
  
  // DMX Frame alle 25ms senden
  if (millis() - dmxTimer >= DMX_INTERVAL) {
    dmxTimer = millis();
    sendDMXFrame();
  }
}

// ====== WiFi AP Setup ======
// IP ADRESSE HIER ÄNDERN (optional):
// Standard: 192.168.4.1
// Beispiele: 192.168.1.100, 10.0.0.1, etc.
IPAddress apIP(192, 168, 4, 1);       // ← IP ADRESSE
IPAddress gateway(192, 168, 4, 1);    // Gateway (normalerweise gleich wie IP)
IPAddress subnet(255, 255, 255, 0);   // Subnet

void setupWiFiAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, gateway, subnet);  // IP setzen VOR softAP()
  WiFi.softAP(SSID_AP, PASSWORD_AP);
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP: ");
  Serial.println(IP);
  Serial.print("Connect to: ");
  Serial.println(SSID_AP);
}

// ====== DMX FRAME SENDEN ======
void sendDMXFrame() {
  // DMX Break Signal + Daten
  digitalWrite(RS485_DE, HIGH);  // Enable Sender
  
  Serial1.flush();
  
  // Start Code (0)
  Serial1.write(0);
  
  // DMX Kanäle 1-512
  for (int i = 0; i < DMX_CHANNELS; i++) {
    Serial1.write(dmxData[i]);
  }
  
  Serial1.flush();
  digitalWrite(RS485_DE, LOW);  // Disable Sender
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
  html += ".display-area { background: #0f0f0f; border: 2px solid #ffb300; border-radius: 5px; padding: 20px; margin-bottom: 25px; box-shadow: inset 0 0 20px rgba(255, 179, 0, 0.1); display: none; }";
  html += ".display-area.show { display: block; }";
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
  html += ".button-grid { display: grid; grid-template-columns: repeat(4, 1fr); gap: 10px; }";
  html += "button { background: #ffb300; color: #000; border: none; padding: 12px; border-radius: 3px; font-weight: bold; cursor: pointer; font-size: 12px; text-transform: uppercase; letter-spacing: 1px; transition: all 0.2s; }";
  html += "button:hover { background: #ffc933; box-shadow: 0 0 15px rgba(255, 179, 0, 0.5); }";
  html += "button:active { transform: scale(0.98); }";
  html += ".info-bar { color: #ffb300; font-size: 11px; margin-top: 15px; text-align: center; border-top: 1px solid #333; padding-top: 10px; }";
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
  html += "</style></head><body>";
  html += "<div class=\"container\">";
  html += "<br><br>";
  html += "<br><br>";
  html += "<br><br>";
  html += "<h1>DMX TESTER</h1>";
  html += "<div class=\"toggle-section\">";
  html += "<button class=\"toggle-btn active\" onclick=\"showPage('control')\">CONTROL</button>";
  html += "<button class=\"toggle-btn\" onclick=\"showPage('display')\">MONITOR</button>";
  html += "<button class=\"toggle-btn\" onclick=\"showPage('colors')\">COLORS</button>";
  html += "</div>";
  html += "<div class=\"display-area\" id=\"displayPage\">";
  html += "<div class=\"display-title\">Channel Display (All 512)</div>";
  html += "<div class=\"channels-display\" id=\"displayArea\"></div>";
  html += "</div>";
  html += "<div id=\"controlPage\">";
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
  html += "<div id=\"colorsPage\" style=\"display:none;\">";
  html += "<div class=\"input-section\">";
  html += "<div class=\"input-title\">UI Color Theme</div>";
  html += "<div class=\"colors-grid\">";
  html += "<button class=\"color-preset\" onclick=\"setColor('#ffb300', 'AMBER')\"><span style=\"background:#ffb300;width:30px;height:30px;border-radius:3px;display:inline-block;border:2px solid #fff;\"></span><br/>AMBER</button>";
  html += "<button class=\"color-preset\" onclick=\"setColor('#00ff00', 'GREEN')\"><span style=\"background:#00ff00;width:30px;height:30px;border-radius:3px;display:inline-block;border:2px solid #fff;\"></span><br/>GREEN</button>";
  html += "<button class=\"color-preset\" onclick=\"setColor('#ff0055', 'PINK')\"><span style=\"background:#ff0055;width:30px;height:30px;border-radius:3px;display:inline-block;border:2px solid #fff;\"></span><br/>PINK</button>";
  html += "<button class=\"color-preset\" onclick=\"setColor('#00ffff', 'CYAN')\"><span style=\"background:#00ffff;width:30px;height:30px;border-radius:3px;display:inline-block;border:2px solid #fff;\"></span><br/>CYAN</button>";
  html += "<button class=\"color-preset\" onclick=\"setColor('#ff6600', 'ORANGE')\"><span style=\"background:#ff6600;width:30px;height:30px;border-radius:3px;display:inline-block;border:2px solid #fff;\"></span><br/>ORANGE</button>";
  html += "<button class=\"color-preset\" onclick=\"setColor('#00ff88', 'MINT')\"><span style=\"background:#00ff88;width:30px;height:30px;border-radius:3px;display:inline-block;border:2px solid #fff;\"></span><br/>MINT</button>";
  html += "<button class=\"color-preset\" onclick=\"setColor('#ff00ff', 'MAGENTA')\"><span style=\"background:#ff00ff;width:30px;height:30px;border-radius:3px;display:inline-block;border:2px solid #fff;\"></span><br/>MAGENTA</button>";
  html += "<button class=\"color-preset\" onclick=\"setColor('#ffff00', 'YELLOW')\"><span style=\"background:#ffff00;width:30px;height:30px;border-radius:3px;display:inline-block;border:2px solid #fff;\"></span><br/>YELLOW</button>";
  html += "<button class=\"color-preset\" onclick=\"setColor('#00ffff', 'BLUE')\"><span style=\"background:#0088ff;width:30px;height:30px;border-radius:3px;display:inline-block;border:2px solid #fff;\"></span><br/>BLUE</button>";
  html += "<button class=\"color-preset\" onclick=\"setColor('#ff4400', 'RED')\"><span style=\"background:#ff4400;width:30px;height:30px;border-radius:3px;display:inline-block;border:2px solid #fff;\"></span><br/>RED</button>";
  html += "</div>";
  html += "<div style=\"margin-top:20px;padding:15px;background:#0f0f0f;border:1px solid #ffb300;border-radius:3px;\">";
  html += "<div class=\"input-label\">Custom Color (HEX)</div>";
  html += "<div style=\"display:flex;gap:10px;margin-top:10px;\">";
  html += "<input type=\"text\" id=\"customColor\" placeholder=\"#ffb300\" maxlength=\"7\" style=\"background:#000;border:1px solid #ffb300;color:#ffb300;padding:8px;flex:1;border-radius:3px;font-family:monospace;\">";
  html += "<button onclick=\"applyCustomColor()\" style=\"background:#ffb300;color:#000;padding:8px 15px;border:none;border-radius:3px;font-weight:bold;cursor:pointer;\">APPLY</button>";
  html += "</div>";
  html += "</div>";
  html += "<div class=\"status-bar\" id=\"colorStatus\" style=\"margin-top:20px;\">Current: AMBER</div>";
  html += "</div>";
  html += "</div>";
  html += "</div></div>";
  html += "<script>";
  html += "const DMX_CHANNELS = 512; let dmxValues = new Array(DMX_CHANNELS).fill(0);";
  html += "let chaseActive = false;";
  html += "function showPage(page) {";
  html += "const displayPage = document.getElementById('displayPage');";
  html += "const controlPage = document.getElementById('controlPage');";
  html += "const colorsPage = document.getElementById('colorsPage');";
  html += "const btns = document.querySelectorAll('.toggle-btn');";
  html += "btns.forEach(b => b.classList.remove('active'));";
  html += "displayPage.classList.remove('show');";
  html += "controlPage.style.display = 'none';";
  html += "colorsPage.style.display = 'none';";
  html += "if (page === 'display') {";
  html += "displayPage.classList.add('show');";
  html += "btns[1].classList.add('active');";
  html += "} else if (page === 'colors') {";
  html += "colorsPage.style.display = 'block';";
  html += "btns[2].classList.add('active');";
  html += "} else {";
  html += "controlPage.style.display = 'block';";
  html += "btns[0].classList.add('active');";
  html += "}";
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
  html += "function setChannelManual() {";
  html += "const ch = document.getElementById('channelInput').value;";
  html += "const val = document.getElementById('valueInput').value;";
  html += "setChannelValue(ch, val);";
  html += "updateStatus('CH ' + ch + ' = ' + val);";
  html += "}";
  html += "function incrementChannel() {";
  html += "const val = Math.min(255, parseInt(document.getElementById('valueInput').value) + 1);";
  html += "document.getElementById('valueInput').value = val;";
  html += "setChannelManual();";
  html += "}";
  html += "function decrementChannel() {";
  html += "const val = Math.max(0, parseInt(document.getElementById('valueInput').value) - 1);";
  html += "document.getElementById('valueInput').value = val;";
  html += "setChannelManual();";
  html += "}";
  html += "function allOn() {";
  html += "for (let i = 0; i < DMX_CHANNELS; i++) dmxValues[i] = 255;";
  html += "updateDisplay();";
  html += "sendDMX();";
  html += "updateStatus('ALL CHANNELS = 255');";
  html += "}";
  html += "function allOff() {";
  html += "for (let i = 0; i < DMX_CHANNELS; i++) dmxValues[i] = 0;";
  html += "updateDisplay();";
  html += "sendDMX();";
  html += "updateStatus('ALL CHANNELS = 0');";
  html += "}";
  html += "function setAllValue(v) {";
  html += "for (let i = 0; i < DMX_CHANNELS; i++) dmxValues[i] = v;";
  html += "updateDisplay();";
  html += "sendDMX();";
  html += "updateStatus('ALL CHANNELS = ' + v);";
  html += "}";
  html += "function resetAll() {";
  html += "for (let i = 0; i < DMX_CHANNELS; i++) dmxValues[i] = 0;";
  html += "updateDisplay();";
  html += "sendDMX();";
  html += "document.getElementById('channelInput').value = 1;";
  html += "document.getElementById('valueInput').value = 0;";
  html += "updateStatus('RESET - ALL OFF');";
  html += "}";
  html += "function chase() {";
  html += "chaseActive = !chaseActive;";
  html += "if (chaseActive) {";
  html += "let ch = 0;";
  html += "const chaseLoop = setInterval(() => {";
  html += "if (!chaseActive) { clearInterval(chaseLoop); return; }";
  html += "dmxValues[ch] = 0;";
  html += "ch = (ch + 1) % DMX_CHANNELS;";
  html += "dmxValues[ch] = 255;";
  html += "updateDisplay();";
  html += "sendDMX();";
  html += "}, 50);";
  html += "updateStatus('CHASE RUNNING...');";
  html += "} else {";
  html += "updateStatus('CHASE STOPPED');";
  html += "}";
  html += "}";
  html += "function sendDMX() {";
  html += "fetch('/api/dmx', {method: 'POST', headers: {'Content-Type': 'application/json'}, body: JSON.stringify({values: dmxValues})});";
  html += "}";
  html += "function loadDMXState() {";
  html += "fetch('/api/dmx').then(r => r.json()).then(d => { dmxValues = d.values; updateDisplay(); });";
  html += "}";
  html += "function loadPreset(n) {";
  html += "fetch('/api/preset?preset=' + n + '&action=load').then(r => r.json()).then(d => {";
  html += "dmxValues = d.values; updateDisplay(); sendDMX();";
  html += "updateStatus('SCENE ' + (n+1) + ' LOADED');";
  html += "});";
  html += "}";
  html += "function updateStatus(msg) {";
  html += "document.getElementById('statusBar').textContent = msg;";
  html += "}";
  html += "document.addEventListener('DOMContentLoaded', function() {";
  html += "initDisplay();";
  html += "initPresets();";
  html += "loadDMXState();";
  html += "setInterval(loadDMXState, 500);";
  html += "document.getElementById('channelInput').addEventListener('keypress', function(e) { if (e.key === 'Enter') setChannelManual(); });";
  html += "document.getElementById('valueInput').addEventListener('keypress', function(e) { if (e.key === 'Enter') setChannelManual(); });";
  html += "});";
  html += "let currentColor = '#ffb300';";
  html += "let currentColorName = 'AMBER';";
  html += "function setColor(hexColor, colorName) {";
  html += "currentColor = hexColor;";
  html += "currentColorName = colorName;";
  html += "applyColorToUI();";
  html += "document.getElementById('colorStatus').textContent = 'Current: ' + colorName;";
  html += "document.getElementById('customColor').value = hexColor;";
  html += "}";
  html += "function applyCustomColor() {";
  html += "const input = document.getElementById('customColor').value.trim();";
  html += "if (/^#[0-9A-F]{6}$/i.test(input)) {";
  html += "setColor(input, 'CUSTOM');";
  html += "} else {";
  html += "alert('Invalid HEX color! Use format: #RRGGBB');";
  html += "}";
  html += "}";
  html += "function applyColorToUI() {";
  html += "const root = document.documentElement;";
  html += "document.body.style.color = currentColor;";
  html += "document.querySelectorAll('.container').forEach(el => {";
  html += "el.style.borderColor = currentColor;";
  html += "el.style.boxShadow = '0 0 30px ' + currentColor + 'cc, 0 0 60px ' + currentColor + '99, inset 0 0 30px ' + currentColor + '1a, inset 0 0 60px ' + currentColor + '0d';";
  html += "});";
  html += "document.querySelectorAll('h1').forEach(el => {";
  html += "el.style.color = currentColor;";
  html += "el.style.textShadow = '0 0 10px ' + currentColor + 'cc, 0 0 20px ' + currentColor + '99, 0 0 30px ' + currentColor + '66, 0 0 40px ' + currentColor + '33';";
  html += "});";
  html += "document.querySelectorAll('.toggle-btn').forEach(el => {";
  html += "el.style.borderColor = currentColor;";
  html += "el.style.color = currentColor;";
  html += "});";
  html += "document.querySelectorAll('.toggle-btn.active').forEach(el => {";
  html += "el.style.backgroundColor = currentColor;";
  html += "el.style.color = '#000';";
  html += "});";
  html += "document.querySelectorAll('.display-title, .input-title, .input-label, .presets-title, .display-title, .channel-display-val').forEach(el => {";
  html += "el.style.color = currentColor;";
  html += "});";
  html += "document.querySelectorAll('.input-section, .display-area').forEach(el => {";
  html += "el.style.borderColor = currentColor;";
  html += "el.style.boxShadow = 'inset 0 0 20px ' + currentColor + '1a';";
  html += "});";
  html += "document.querySelectorAll('.status-bar, #statusBar, #colorStatus').forEach(el => {";
  html += "el.style.color = currentColor;";
  html += "el.style.borderColor = currentColor;";
  html += "});";
  html += "document.querySelectorAll('input[type=\"number\"]').forEach(el => {";
  html += "el.style.borderColor = currentColor;";
  html += "el.style.color = currentColor;";
  html += "});";
  html += "document.querySelectorAll('.preset-btn, .color-preset').forEach(el => {";
  html += "el.style.borderColor = currentColor;";
  html += "el.style.color = currentColor;";
  html += "});";
  html += "}";
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

// ====== WEB HANDLER - SEND ======
void handleSendDMX() {
  server.send(200, "application/json", "{\"status\":\"Sending\"}");
}

// ====== EEPROM FUNKTIONEN ======
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
