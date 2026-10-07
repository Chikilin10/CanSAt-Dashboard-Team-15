import useTelemetry, { useTelemetryStore } from "../../hooks/useTelemetry";

const COLORS = { connected: "success", connecting: "warning", paused: "warning", disconnected: "danger", idle: "secondary" };
const LABELS = { connected: "Conectado", connecting: "Conectando…", paused: "Pausado", disconnected: "Desconectado", idle: "Inactivo" };

export default function StatusBar() {
  const store = useTelemetryStore();
  const { last, status, received, firstPacket } = useTelemetry();
  const paused = status === "paused";

  const expected = last ? last.packet - firstPacket + 1 : 0;
  const loss = expected > 0 ? Math.max(0, (1 - received / expected) * 100) : 0;
  const items = [
    ["Paquete", last?.packet ?? "—"],
    ["Tiempo", last ? `${last.t.toFixed(1)} s` : "—"],
    ["Fase", last?.phase ?? "Pendiente"],
    ["Pérdida", `${loss.toFixed(1)} %`],
  ];

  return (
    <div className="card shadow-sm border-0 mb-3">
      <div className="card-body d-flex flex-wrap gap-4 align-items-center py-2">
        <span className={`badge text-bg-${COLORS[status]}`}>{LABELS[status]}</span>
        {items.map(([k, v]) => (
          <div key={k}>
            <small className="text-muted d-block">{k}</small>
            <strong>{v}</strong>
          </div>
        ))}
        <div className="ms-auto d-flex gap-2">
          <button className="btn btn-sm btn-outline-primary" onClick={() => (paused ? store.resume() : store.pause())}>
            {paused ? "Reanudar" : "Pausar"}
          </button>
          <button className="btn btn-sm btn-outline-secondary" onClick={() => store.clear()}>
            Limpiar
          </button>
        </div>
      </div>
    </div>
  );
}