import TelemetrySource from "./TelemetrySource";
import TelemetryPacket from "../models/TelemetryPacket";

// Fuente real: WebSocket hacia el puente Node (pendiente) con reconexión.
export default class WsSource extends TelemetrySource {
  #url;
  #ws = null;
  #retry = 0;
  #timer = null;
  #stopped = true;

  constructor(url) {
    super();
    this.#url = url;
  }

  start() {
    if (!this.#stopped) return;
    this.#stopped = false;
    this.#connect();
  }

  stop() {
    this.#stopped = true;
    clearTimeout(this.#timer);
    this.#ws?.close();
    this.setStatus("disconnected");
  }

  #connect() {
    this.setStatus("connecting");
    const ws = (this.#ws = new WebSocket(this.#url));
    ws.onopen = () => {
      this.#retry = 0;
      this.setStatus("connected");
    };
    ws.onmessage = (e) => {
      try {
        this.emit(TelemetryPacket.fromJSON(e.data));
      } catch (err) {
        console.warn("Paquete inválido:", err);
      }
    };
    ws.onclose = () => {
      if (this.#stopped) return;
      this.setStatus("disconnected");
      const wait = Math.min(5000, 500 * 2 ** this.#retry++);
      this.#timer = setTimeout(() => this.#connect(), wait);
    };
  }
}
