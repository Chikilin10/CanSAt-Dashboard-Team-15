import CardShell from "./CardShell";
import PendingValue from "../common/PendingValue";

export default function ProgressCard({ value, max, unit, decimals = 1, ...shell }) {
  const pending = value == null;
  const pct = pending ? 0 : Math.min(100, Math.max(0, (value / max) * 100));
  return (
    <CardShell {...shell} pending={pending}>
      <div className="fs-3 fw-bold text-center">
        <PendingValue value={value} decimals={decimals} />
        <small className="fs-6 text-muted ms-1">/ {max} {unit}</small>
      </div>
      <div className="progress" style={{ height: 10 }}>
        <div className="progress-bar" style={{ width: `${pct}%` }} />
      </div>
    </CardShell>
  );
}
