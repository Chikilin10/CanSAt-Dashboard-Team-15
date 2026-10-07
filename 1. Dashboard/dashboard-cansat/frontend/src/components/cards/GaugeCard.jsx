import CardShell from "./CardShell";
import PendingValue from "../common/PendingValue";

const pt = (cx, cy, r, t) => [cx + r * Math.cos(Math.PI * (1 - t)), cy - r * Math.sin(Math.PI * (1 - t))];
const arc = (cx, cy, r, t0, t1) => {
  const [x0, y0] = pt(cx, cy, r, t0);
  const [x1, y1] = pt(cx, cy, r, t1);
  return `M ${x0} ${y0} A ${r} ${r} 0 0 1 ${x1} ${y1}`;
};

export default function GaugeCard({ value, min = 0, max = 20, okRange, unit, decimals = 1, ...shell }) {
  const pending = value == null;
  const norm = (v) => Math.min(1, Math.max(0, (v - min) / (max - min)));
  const [nx, ny] = pt(100, 100, 68, pending ? 0 : norm(value));
  return (
    <CardShell {...shell} pending={pending}>
      <svg viewBox="0 0 200 110" className="w-100">
        <path d={arc(100, 100, 80, 0, 1)} stroke="#e9ecef" strokeWidth="14" fill="none" />
        {okRange && (
          <path d={arc(100, 100, 80, norm(okRange[0]), norm(okRange[1]))} stroke="#198754" strokeWidth="14" fill="none" />
        )}
        {!pending && <line x1="100" y1="100" x2={nx} y2={ny} stroke="#212529" strokeWidth="3" strokeLinecap="round" />}
        <circle cx="100" cy="100" r="5" fill="#212529" />
      </svg>
      <div className="fs-3 fw-bold text-center">
        <PendingValue value={value} decimals={decimals} />
        {!pending && <small className="fs-6 text-muted ms-1">{unit}</small>}
      </div>
    </CardShell>
  );
}
