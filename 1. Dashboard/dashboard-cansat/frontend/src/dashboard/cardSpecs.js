import CardSpec from "./CardSpec";

// Para agregar o quitar tarjetas, solo se edita esta lista.
export const CARD_SPECS = [
  new CardSpec({ id: "alt", type: "spark", field: "alt", label: "Altitud", unit: "m", source: "Calculado (C)", color: "#0d6efd" }),
  new CardSpec({ id: "vel", type: "spark", field: "vel_v", label: "Velocidad vertical", unit: "m/s", source: "Calculado (C)", color: "#6f42c1" }),
  new CardSpec({ id: "temp", type: "number", field: "temp", label: "Temperatura", unit: "°C", source: "Sensor" }),
  new CardSpec({ id: "pressure", type: "number", field: "pressure", label: "Presión", unit: "hPa", source: "Sensor" }),
  new CardSpec({
    id: "volt", type: "progress", field: "volt", label: "Batería 9V", unit: "V", source: "Sensor", decimals: 2, max: 9.6,
    // El regulador a 5V necesita ~6.5 V como mínimo; una 9V alcalina cae rápido bajo carga.
    statusFn: (l) =>
      l?.volt == null ? {} : l.volt < 6.8 ? { status: "alert", badgeText: "Baja" } : l.volt < 7.6 ? { status: "warn", badgeText: "Media" } : {},
  }),
  new CardSpec({
    id: "descent", type: "gauge", field: "descent_rate", label: "Velocidad de caída",
    unit: "m/s", source: "Calculado (C)", min: 0, max: 40, okRange: [8, 12],
    statusFn: (l) =>
      l?.descent_ok == null
        ? {}
        : l.descent_ok
          ? { status: "ok", badgeText: "En rango" }
          : { status: "alert", badgeText: "Fuera de 8–12" },
  }),
  new CardSpec({ id: "post", type: "progress", field: "post_landing_s", label: "Post-aterrizaje", unit: "s", source: "Calculado (C)", max: 10 }),
  new CardSpec({ id: "impact", type: "number", field: "impact_g", label: "Impacto", unit: "g", source: "Calculado (C)" }),
];
