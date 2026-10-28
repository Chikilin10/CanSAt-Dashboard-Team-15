import { useState } from "react";
import useTelemetry, { useTelemetryStore } from "../../hooks/useTelemetry";

// Comandos que entiende el puente (backend/bridge) y el CanSat. Ver docs/protocol.md.
const COMMANDS = [
  {
    cmd: "DEPLOY",
    label: "Soltar carga secundaria",
    variant: "danger",
    confirm: "¿Soltar la carga secundaria AHORA?\nEl servo se activa y no se puede deshacer en vuelo.",
  },
  { cmd: "TARE", label: "Altura = 0", variant: "outline-primary", confirm: "Solo en tierra: toma la altura actual como 0 m. ¿Continuar?" },
  { cmd: "LOCK", label: "Bloquear servo", variant: "outline-secondary", confirm: "Vuelve a bloquear el servo y reinicia el estado del CanSat (solo pruebas en mesa). ¿Continuar?" },
  { cmd: "RESET", label: "Reiniciar cálculo", variant: "outline-secondary", confirm: null },
];

export default function CommandBar() {
  const store = useTelemetryStore();
  const { status } = useTelemetry();
  const [feedback, setFeedback] = useState(null);

  const run = ({ cmd, label, confirm }) => {
    if (confirm && !window.confirm(confirm)) return;
    const sent = store.sendCommand(cmd);
    setFeedback(sent ? `✔ ${label}: enviado` : `✖ ${label}: no se envió (¿sin conexión o en modo simulación?)`);
  };

  return (
    <div className="card shadow-sm border-0 mb-3">
      <div className="card-body d-flex flex-wrap gap-2 align-items-center py-2">
        <small className="text-muted me-2">Comandos</small>
        {COMMANDS.map((c) => (
          <button key={c.cmd} className={`btn btn-sm btn-${c.variant}`} disabled={status !== "connected"} onClick={() => run(c)}>
            {c.label}
          </button>
        ))}
        {feedback && <small className="ms-2 text-muted">{feedback}</small>}
      </div>
    </div>
  );
}
