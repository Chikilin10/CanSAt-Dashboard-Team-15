const COLORS = { ok: "success", warn: "warning", alert: "danger", pending: "secondary" };

export default function CardShell({ label, source, status = "ok", badgeText, pending = false, children }) {
  const key = pending ? "pending" : status;
  return (
    <div className="card h-100 shadow-sm border-0">
      <div className="card-body d-flex flex-column">
        <small className="text-muted">{label}</small>
        <div className="flex-grow-1 d-flex flex-column justify-content-center my-2">{children}</div>
        <div className="d-flex justify-content-between align-items-center">
          <small className="text-muted">{source}</small>
          <span className={`badge text-bg-${COLORS[key]}`}>
            {pending ? "Pendiente" : badgeText ?? "OK"}
          </span>
        </div>
      </div>
    </div>
  );
}
