// Un paquete = datos crudos (CanSat) + datos derivados (backend en C, pendiente).
export default class TelemetryPacket {
  #raw;
  #derived;

  constructor(raw = {}, derived = {}) {
    this.#raw = raw;
    this.#derived = derived;
  }

  static fromJSON(text) {
    const { raw, derived } = JSON.parse(text);
    return new TelemetryPacket(raw, derived);
  }

  // Objeto plano: las tarjetas leen last.alt, last.temp, etc.
  flatten() {
    return { ...this.#raw, ...this.#derived };
  }
}
