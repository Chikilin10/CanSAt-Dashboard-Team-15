/*
 * Puente CanSat
 *   ESP32 de tierra --USB serial--> [este programa] --stdin--> backend en C
 *   backend en C --stdout (JSON)--> [este programa] --WebSocket--> dashboard
 *   dashboard --WebSocket (comandos)--> [este programa] --serial--> ESP32 de tierra --LoRa--> CanSat
 *
 * Variables de entorno:
 *   SERIAL_PATH  puerto (COM3, /dev/ttyUSB0, /dev/cu.usbserial-XXXX). Si no se pone, se intenta detectar.
 *   SOURCE=stdin lee lineas desde stdin (para probar con tools/sim_flight.py, sin hardware)
 *   BAUD (115200)   WS_PORT (8080)
 */
const { spawn } = require("child_process");
const fs = require("fs");
const path = require("path");
const readline = require("readline");
const { WebSocketServer } = require("ws");

const BAUD = Number(process.env.BAUD || 115200);
const WS_PORT = Number(process.env.WS_PORT || 8080);
const SOURCE = process.env.SOURCE || "serial";
const C_BIN = path.join(__dirname, "..", "c", "build", process.platform === "win32" ? "cansat_backend.exe" : "cansat_backend");
const DATA_DIR = path.join(__dirname, "..", "..", "data");

// Comandos que el dashboard puede mandar. Todo lo demás se ignora.
const RADIO_COMMANDS = new Set(["DEPLOY", "LOCK", "TARE"]); // viajan por LoRa al CanSat
const LOCAL_COMMANDS = new Set(["RESET"]);                  // los atiende el backend en C
const COMMAND_REPEATS = 3;                                  // LoRa es half-duplex: se repite por si coincide con una transmisión

// ---------- Registro de vuelo (CSV crudo, respaldo para analizar después) ----------
fs.mkdirSync(DATA_DIR, { recursive: true });
const stamp = new Date().toISOString().replace(/[:.]/g, "-");
const logFile = fs.createWriteStream(path.join(DATA_DIR, `vuelo-${stamp}.csv`), { flags: "a" });    // crudo + hora de la PC
const jsonLog = fs.createWriteStream(path.join(DATA_DIR, `vuelo-${stamp}.jsonl`), { flags: "a" });  // crudo + derivado (para el RAM)

// ---------- WebSocket hacia el dashboard ----------
const wss = new WebSocketServer({ port: WS_PORT });
console.log(`[ws] escuchando en ws://localhost:${WS_PORT}`);

function broadcast(text) {
  jsonLog.write(text + "\n");
  for (const client of wss.clients) if (client.readyState === 1) client.send(text);
}

// ---------- Backend en C ----------
let cProc = null;

function startBackend() {
  if (!fs.existsSync(C_BIN)) {
    console.error(`[c] no existe ${C_BIN}\n    Compila primero:  cd backend/c && make`);
    process.exit(1);
  }
  cProc = spawn(C_BIN, [], { stdio: ["pipe", "pipe", "inherit"] });
  readline.createInterface({ input: cProc.stdout }).on("line", broadcast);
  cProc.on("exit", (code) => {
    console.error(`[c] terminó (código ${code}); reiniciando en 1 s`);
    setTimeout(startBackend, 1000);
  });
  cProc.stdin.on("error", () => {}); // si el proceso muere, el 'exit' lo reinicia
}

function feedBackend(line) {
  if (cProc && cProc.stdin.writable) cProc.stdin.write(line + "\n");
}

// ---------- Entrada de datos (línea por línea) ----------
let sendToRadio = () => console.warn("[radio] sin puerto serial: comando no enviado");

function handleLine(raw) {
  const line = raw.trim();
  if (!line) return;
  if (line.startsWith("T,")) {
    logFile.write(`${Date.now()},${line}\n`);
    feedBackend(line);
  } else {
    console.log(`[radio] ${line}`); // mensajes de diagnóstico del ESP32
  }
}

async function openSerial() {
  const { SerialPort, ReadlineParser } = require("serialport");

  let portPath = process.env.SERIAL_PATH;
  if (!portPath) {
    const ports = await SerialPort.list();
    const guess = ports.find((p) => /usb|acm|com/i.test(p.path) && p.vendorId) || ports[0];
    if (!guess) throw new Error("No encontré ningún puerto serial. Define SERIAL_PATH.");
    portPath = guess.path;
    console.log(`[serial] puerto detectado: ${portPath}`);
  }

  const port = new SerialPort({ path: portPath, baudRate: BAUD });
  port.pipe(new ReadlineParser({ delimiter: "\n" })).on("data", handleLine);
  port.on("open", () => {
    console.log(`[serial] abierto ${portPath} @ ${BAUD}`);
    sendToRadio = (cmd) => port.write(cmd + "\n");
  });
  port.on("error", (e) => console.error("[serial] error:", e.message));
  port.on("close", () => {
    console.error("[serial] cerrado; reintentando en 2 s");
    sendToRadio = () => console.warn("[radio] puerto cerrado");
    setTimeout(() => openSerial().catch((e) => console.error(e.message)), 2000);
  });
}

// ---------- Comandos del dashboard ----------
wss.on("connection", (ws) => {
  ws.on("message", (msg) => {
    const cmd = msg.toString().trim().toUpperCase();
    if (LOCAL_COMMANDS.has(cmd)) return feedBackend(cmd);
    // Si el CanSat se vuelve a poner en "0 m", el backend en C debe recalibrar igual.
    if (cmd === "TARE" || cmd === "LOCK") feedBackend("RESET");
    if (!RADIO_COMMANDS.has(cmd)) return console.warn(`[ws] comando ignorado: ${cmd}`);
    console.log(`[ws] comando → CanSat: ${cmd}`);
    for (let i = 0; i < COMMAND_REPEATS; i++) setTimeout(() => sendToRadio(cmd), i * 400);
  });
});

// ---------- Arranque ----------
startBackend();
if (SOURCE === "stdin") {
  console.log("[src] leyendo líneas desde stdin (modo simulación)");
  readline.createInterface({ input: process.stdin }).on("line", handleLine);
} else {
  openSerial().catch((e) => { console.error(e.message); process.exit(1); });
}
