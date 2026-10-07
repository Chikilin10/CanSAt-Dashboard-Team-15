import CardShell from "./CardShell";
import PendingValue from "../common/PendingValue";

export default function NumberCard({ value, unit, decimals = 1, ...shell }) {
  const pending = value == null;
  return (
    <CardShell {...shell} pending={pending}>
      <div className="display-5 fw-bold text-center">
        <PendingValue value={value} decimals={decimals} />
        {!pending && <small className="fs-6 text-muted ms-1">{unit}</small>}
      </div>
    </CardShell>
  );
}
