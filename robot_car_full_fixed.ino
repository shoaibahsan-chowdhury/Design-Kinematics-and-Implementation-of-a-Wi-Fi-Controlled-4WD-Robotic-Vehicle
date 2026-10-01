#include <WiFi.h>
#include <WebServer.h>

// Motor pins — in1/in2 = Left motor, in3/in4 = Right motor
const int in1 = D5;
const int in2 = D6;
const int in3 = D7;
const int in4 = D8;

// ----- SPEED ADJUSTMENTS -----
const int SPEED = 122;       // Bumped up slightly from 122 so straight driving doesn't stall
const int TURN_SPEED = 511;  // FULL POWER (Max 1023) to force the wheels to skid-turn
// -----------------------------

const char* ssid     = "car";
const char* password = "12345678";

WebServer server(80);

// ====================
// Motor Functions
// ====================

void Stop() {
  Serial.println("----STOP----");
  digitalWrite(in1, LOW);  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);  digitalWrite(in4, LOW); 
  digitalWrite(D1, LOW);
}

void Forward() {
  Serial.println("Forward");
  analogWrite(in1, SPEED);
  digitalWrite(in2, LOW);
  analogWrite(in3, SPEED);
  digitalWrite(in4, LOW);
  digitalWrite(D1, HIGH);
}

void Backward() {
  Serial.println("Backward");
  digitalWrite(in1, LOW);
  analogWrite(in2, SPEED);
  digitalWrite(in3, LOW);
  analogWrite(in4, SPEED);
  digitalWrite(D1, HIGH);
}

void TurnLeft() {
  Serial.println("Left");
  analogWrite(in1, TURN_SPEED); // Max power left side backward
  digitalWrite(in2, LOW);
  digitalWrite(in3, LOW);
  analogWrite(in4, TURN_SPEED); // Max power right side forward
  digitalWrite(D1, HIGH);
}

void TurnRight() {
  Serial.println("Right");
  digitalWrite(in1, LOW);
  analogWrite(in2, TURN_SPEED); // Max power left side forward
  analogWrite(in3, TURN_SPEED); // Max power right side backward
  digitalWrite(in4, LOW);
  digitalWrite(D1, HIGH);
}

// ====================
// Web Page + Command Handler
// ====================

void handleRoot() {
  String state = server.arg("State");
  if      (state == "F") Forward();
  else if (state == "B") Backward();
  else if (state == "L") TurnLeft();
  else if (state == "R") TurnRight();
  else if (state == "S") Stop();

  server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
<title>RC Car</title>
<style>
  *, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }

  body {
    background: #111;
    height: 100dvh;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    font-family: monospace;
    touch-action: none;
    user-select: none;
    -webkit-user-select: none;
    overflow: hidden;
  }

  .dpad {
    display: grid;
    grid-template-columns: repeat(3, 80px);
    grid-template-rows: repeat(3, 80px);
    gap: 8px;
  }

  .btn {
    background: #1c1c1c;
    border: 1px solid #2e2e2e;
    border-radius: 10px;
    color: #ddd;
    font-size: 28px;
    display: flex;
    align-items: center;
    justify-content: center;
    cursor: pointer;
    -webkit-tap-highlight-color: transparent;
  }

  .btn.active {
    background: #2a2a2a;
    border-color: #666;
  }

  #F { grid-column:2; grid-row:1; }
  #L { grid-column:1; grid-row:2; }
  #R { grid-column:3; grid-row:2; }
  #B { grid-column:2; grid-row:3; }
</style>
</head>
<body>

<div class="dpad">
  <button class="btn" id="F">&#9650;</button>
  <button class="btn" id="L">&#9664;</button>
  <button class="btn" id="R">&#9654;</button>
  <button class="btn" id="B">&#9660;</button>
</div>

<script>
  const ids = ['F', 'B', 'L', 'R'];
  let active = null;

  function send(cmd) {
    fetch('/?State=' + cmd).catch(() => {});
  }

  function press(id) {
    if (id === active) return;
    active = id;
    ids.forEach(i => document.getElementById(i).classList.remove('active'));
    document.getElementById(id).classList.add('active');
    send(id);
  }

  function release() {
    if (!active) return;
    active = null;
    ids.forEach(i => document.getElementById(i).classList.remove('active'));
    send('S');
  }

  ids.forEach(id => {
    const el = document.getElementById(id);
    el.addEventListener('touchstart', e => { e.preventDefault(); press(id); }, { passive: false });
    el.addEventListener('touchend',   e => { e.preventDefault(); release(); }, { passive: false });
    el.addEventListener('mousedown',  () => press(id));
    el.addEventListener('mouseup',    () => release());
    el.addEventListener('mouseleave', () => { if (active === id) release(); });
  });

  const keyMap = { ArrowUp:'F', ArrowDown:'B', ArrowLeft:'L', ArrowRight:'R' };
  document.addEventListener('keydown', e => { if (keyMap[e.key]) { e.preventDefault(); press(keyMap[e.key]); } });
  document.addEventListener('keyup',   e => { if (keyMap[e.key]) release(); });
</script>

</body>
</html>
)rawliteral");
}

// ====================
// Setup
// ====================

void setup() {
  Serial.begin(9600);

  pinMode(in1, OUTPUT); pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT); pinMode(in4, OUTPUT);
  pinMode(D1, OUTPUT);
  Stop();

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println("");
  Serial.println("AP IP: " + WiFi.softAPIP().toString());

  server.on("/", handleRoot);
  server.onNotFound([]() { handleRoot(); });
  server.begin();
}

// ====================
// Loop
// ====================

void loop() {
  server.handleClient();
}