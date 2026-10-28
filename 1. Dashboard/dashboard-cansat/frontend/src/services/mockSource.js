import TelemetrySource from "./TelemetrySource";
import TelemetryPacket from "../models/TelemetryPacket";
import config from "../config";

// Simula un vuelo completo: suelo → ascenso (450 m) → caída libre
// → autogiro (200 m) → aterrizaje → 15 s de transmisión → reinicia.
export default class MockSource extends TelemetrySource {
  #timer = null;
  #includeDerived;
  #dt = config.mockIntervalMs / 1000;
  #n = 0;
  #t = 0;
  #s;

  // includeDerived=false simula que el backend en C aún no existe.
  constructor({ includeDerived = true } = {}) {
    super();
    this.#includeDerived = includeDerived;
    this.#reset();
  }

  #reset() {
    this.#s = { phase: "ground", alt: 0, vel: 0, altMax: 0, deployAlt: null, impactG: null, landedAt: null };
    this.#t = 0;
  }

  start() {
    if (this.#timer) return;
    this.setStatus("connected");
    this.#timer = setInterval(() => this.#tick(), config.mockIntervalMs);
  }

  stop() {
    clearInterval(this.#timer);
    this.#timer = null;
    this.setStatus("disconnected");
  }

  #advance() {
    const s = this.#s;
    const dt = this.#dt;
    switch (s.phase) {
      case "ground":
        if (this.#t > 4) s.phase = "ascent";
        break;
      case "ascent":
        s.vel = 30;
        s.alt += s.vel * dt;
        if (s.alt >= 450) s.phase = "freefall";
        break;
      case "freefall":
        s.vel = Math.max(-35, s.vel - 9.8 * dt);
        s.alt += s.vel * dt;
        if (s.alt <= 200) { s.phase = "autogyro"; s.deployAlt = s.alt; }
        break;
      case "autogyro":
        s.vel += (-10 - s.vel) * 0.15;
        s.alt += s.vel * dt;
        if (s.alt <= 0) {
          Object.assign(s, { alt: 0, vel: 0, phase: "landed", landedAt: this.#t, impactG: 6.5 });
        }
        break;
      case "landed":
        if (this.#t - s.landedAt > 15) this.#reset();
        break;
    }
    s.altMax = Math.max(s.altMax, s.alt);
  }

  #tick() {
    this.#t += this.#dt;
    this.#advance();
    const { phase, alt, vel, altMax, deployAlt, impactG, landedAt } = this.#s;
    const noise = (a) => (Math.random() - 0.5) * a;
    const landed = phase === "landed";

    const raw = {
      packet: this.#n++,
      t: +this.#t.toFixed(2),
      pressure: +(1013.25 * (1 - alt / 44330) ** 5.255 + noise(0.3)).toFixed(2),
      temp: +(25 - 0.0065 * alt + noise(0.2)).toFixed(2),
      ax: +noise(0.4).toFixed(2),
      ay: +noise(0.4).toFixed(2),
      az: +((phase === "freefall" ? 0.3 : 9.8) + noise(0.4)).toFixed(2),
      gx: +noise(6).toFixed(1),
      gy: +noise(6).toFixed(1),
      gz: +noise(6).toFixed(1),
      volt: +(9.3 - this.#n * 0.0006).toFixed(2),
      rotor_deployed: phase === "autogyro" || landed,
      sec_dist: landed ? 4.2 : null,
      sec_bearing: landed ? 135 : null,
    };

    const rate = phase === "autogyro" ? +(-vel).toFixed(1) : null;
    const derived = this.#includeDerived
      ? {
          alt: +alt.toFixed(1),
          vel_v: +vel.toFixed(1),
          alt_max: +altMax.toFixed(1),
          phase,
          deploy_alt: deployAlt && +deployAlt.toFixed(1),
          descent_rate: rate,
          descent_ok: rate === null ? null : rate >= 8 && rate <= 12,
          impact_g: impactG,
          post_landing_s: landed ? +(this.#t - landedAt).toFixed(1) : null,
        }
      : {};

    this.emit(new TelemetryPacket(raw, derived));
  }
}
