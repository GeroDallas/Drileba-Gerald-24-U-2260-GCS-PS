#include <WiFi.h>
#include <WebServer.h>
#include <time.h>

// Wi-Fi credentials
const char *ssid = "@dallas";
const char *password = "dallas@21";

// Authentication
const char *ADMIN_USER = "DRILEBA";
const char *ADMIN_PASS = "gerald";

// LED pins
int ledPins[3] = {31,30,29};
bool ledState[3] = {false, false, false};
bool loggedIn = false;

WebServer server(80);

// HTML Header
String htmlHeader(const String &title) {
  String h = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'>";
  h += "<title>" + title + "</title>";
  h += "<style>";
  h += "body{font-family:sans-serif;margin:0;padding:0;background:#eef3f7;color:#333;text-align:center;}";
  h += "h1{background:#004466;color:white;padding:20px 10px;margin:0;font-size:24px;}";
  h += ".container{padding:20px;}";
  h += "button { padding: 10px 20px; font-size: 16px; margin: 5px; cursor: pointer; border: none; }";
  h += ".on { padding: 10px 20px; font-size: 16px; margin: 5px; cursor: pointer; border: none; }";
  h += ".off { padding: 10px 20px; font-size: 16px; margin: 5px; cursor: pointer; border: none; }";
  h += ".logout { padding: 10px 20px; font-size: 16px; margin: 5px; cursor: pointer; border: none; }";
  h += ".status{font-weight:bold;}";
  h += "@media(max-width:600px){button{width:90%;margin:8px auto;display:block;}}";
  h += "footer{margin-top:40px;font-size:13px;color:#666;}";
  h += "</style></head><body>";
  return h;
}

// HTML Footer
String htmlFooter() {
  return "<footer>Drileba Gerald systems</footer></body></html>";
}

// Login Page
String loginPage(const String &msg = "") {
  String html = htmlHeader("Login");
  html += "<h1>Team Logic Loopers</h1><div class='container'>";
  if (msg.length()) html += "<p style='color:red;'>" + msg + "</p>";
  html += "<form method='POST' action='/login'>";
  html += "<input type='text' name='username' placeholder='Username' required><br><br>";
  html += "<input type='password' name='password' placeholder='Password' required><br><br>";
  html += "<input type='submit' value='Login' class='on'>";
  html += "</form></div>" + htmlFooter();
  return html;
}

// Dashboard
String controlPage() {
  String html = htmlHeader("Dashboard");
  html += "<h1>Logic Loopers - Control Panel</h1><div class='container'>";
  html += "<h3>Main Control</h3>";
  html += "<a href='/allon'><button class='on'>Turn all ON</button></a>";
  html += "<a href='/alloff'><button class='off'>Turn all OFF</button></a><br><br>";

  html += "<h3>Individual Lights</h3>";
  for (int i = 0; i < 3; i++) {
    html += "<p>Light " + String(i + 1) + ": ";
    html += "<span class='status'>" + String(ledState[i] ? "ON" : "OFF") + "</span><br>";
    html += "<a href='/on?led=" + String(i) + "'><button class='on'>ON</button></a>";
    html += "<a href='/off?led=" + String(i) + "'><button class='off'>OFF</button></a></p>";
  }

  html += "<br><a href='/logout'><button class='logout'>Logout</button></a></div>" + htmlFooter();
  return html;
}

bool ensureAuthed() {
  if (!loggedIn) {
    server.send(200, "text/html", loginPage());
    return false;
  }
  return true;
}

// Web Handlers
void handleRoot() {
  if (!ensureAuthed()) return;
  server.send(200, "text/html", controlPage());
}

void handleLogin() {
  if (server.method() == HTTP_POST) {
    String user = server.arg("username");
    String pass = server.arg("password");
    if (user == ADMIN_USER && pass == ADMIN_PASS) {
      loggedIn = true;
      server.sendHeader("Location", "/");
      server.send(303);
    } else {
      server.send(200, "text/html", loginPage("Invalid credentials."));
    }
  } else {
    server.send(200, "text/html", loginPage());
  }
}

void handleLogout() {
  loggedIn = false;
  server.send(200, "text/html", loginPage("You have been logged out."));
}

void handleOn() {
  if (!ensureAuthed()) return;
  int idx = server.arg("led").toInt();
  if (idx >= 0 && idx < 3) {
    digitalWrite(ledPins[idx], HIGH);
    ledState[idx] = true;
  }
  server.send(200, "text/html", controlPage());
}

void handleOff() {
  if (!ensureAuthed()) return;
  int idx = server.arg("led").toInt();
  if (idx >= 0 && idx < 3) {
    digitalWrite(ledPins[idx], LOW);
    ledState[idx] = false;
  }
  server.send(200, "text/html", controlPage());
}

void handleAllOn() {
  if (!ensureAuthed()) return;
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPins[i], HIGH);
    ledState[i] = true;
  }
  server.send(200, "text/html", controlPage());
}

void handleAllOff() {
  if (!ensureAuthed()) return;
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledPins[i], LOW);
    ledState[i] = false;
  }
  server.send(200, "text/html", controlPage());
}

void handleNotFound() {
  server.send(404, "text/plain", "404 Not Found");
}

// Auto Light Control (6:30 PM to 12:30 AM)
void autoLightingControl() {
  time_t now = time(nullptr);
  struct tm *timeinfo = localtime(&now);
  int hour = timeinfo->tm_hour;
  int minute = timeinfo->tm_min;

  // ON at 18:30 (6:30 PM)
  if (hour == 18 && minute == 30) {
    for (int i = 0; i < 3; i++) {
      digitalWrite(ledPins[i], HIGH);
      ledState[i] = true;
    }
  }

  // OFF at 6:30 AM
  if (hour == 6 && minute == 30) {
    for (int i = 0; i < 3; i++) {
      digitalWrite(ledPins[i], LOW);
      ledState[i] = false;
    }
  }
}

// Setup
void setup() {
  Serial.begin(115200);
  for (int i = 0; i < 3; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected! IP address: ");
  Serial.println(WiFi.localIP());

  configTime(3 * 3600, 0, "pool.ntp.org");  // EAT (UTC+3)

  server.on("/", handleRoot);
  server.on("/login", handleLogin);
  server.on("/logout", handleLogout);
  server.on("/on", handleOn);
  server.on("/off", handleOff);
  server.on("/allon", handleAllOn);
  server.on("/alloff", handleAllOff);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("Web server running...");
}

// Loop
void loop() {
  server.handleClient();
  autoLightingControl();
}
