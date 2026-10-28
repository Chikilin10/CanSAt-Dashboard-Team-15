// Guarda el buffer de paquetes y avisa a React cuando cambia.
export default class TelemetryStore {
  #source;
  #max;
  #buffer = [];
  #received = 0;
  #firstPacket = null;
  #status;
  #snapshot;
  #listeners = new Set();

  constructor(source, max = 300) {
    this.#source = source;
    this.#max = max;
    this.#status = source.status;
    this.#refresh();
    source.subscribe((packet) => {
      const flat = packet.flatten();
      this.#firstPacket ??= flat.packet;
      this.#buffer = [...this.#buffer.slice(-(this.#max - 1)), flat];
      this.#received += 1;
      this.#notify();
    });
    source.subscribeStatus((status) => {
      this.#status = status;
      this.#notify();
    });
  }

  #refresh() {
    this.#snapshot = {
      data: this.#buffer,
      last: this.#buffer.at(-1) ?? null,
      status: this.#status,
      received: this.#received,
      firstPacket: this.#firstPacket,
    };
  }

  #notify() {
    this.#refresh();
    this.#listeners.forEach((l) => l());
  }

  #resetCounters() {
    this.#received = 0;
    this.#firstPacket = null;
  }

  subscribe = (listener) => {
    this.#listeners.add(listener);
    return () => this.#listeners.delete(listener);
  };

  getSnapshot = () => this.#snapshot;

  start() {
    this.#source.start();
  }

  stop() {
    this.#source.stop();
  }

  sendCommand(cmd) {
    return this.#source.send(cmd);
  }

  pause() {
    this.#source.stop();
    this.#status = "paused";
    this.#notify();
  }

  resume() {
    this.#resetCounters();
    this.#source.start();
  }

  clear() {
    this.#buffer = [];
    this.#resetCounters();
    this.#notify();
  }
}