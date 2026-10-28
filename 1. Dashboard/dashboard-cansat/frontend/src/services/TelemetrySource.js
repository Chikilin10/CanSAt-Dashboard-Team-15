// Clase base abstracta: toda fuente de datos hereda de aquí.
export default class TelemetrySource {
  #packetListeners = new Set();
  #statusListeners = new Set();
  #status = "idle";

  get status() {
    return this.#status;
  }

  subscribe(cb) {
    this.#packetListeners.add(cb);
    return () => this.#packetListeners.delete(cb);
  }

  subscribeStatus(cb) {
    this.#statusListeners.add(cb);
    return () => this.#statusListeners.delete(cb);
  }

  emit(packet) {
    this.#packetListeners.forEach((cb) => cb(packet));
  }

  setStatus(status) {
    this.#status = status;
    this.#statusListeners.forEach((cb) => cb(status));
  }

  // Envía un comando al CanSat. Devuelve true si salió. La fuente simulada no tiene a quién enviarlo.
  send() {
    return false;
  }

  start() {
    throw new Error("start() debe implementarse en la subclase");
  }

  stop() {
    throw new Error("stop() debe implementarse en la subclase");
  }
}
