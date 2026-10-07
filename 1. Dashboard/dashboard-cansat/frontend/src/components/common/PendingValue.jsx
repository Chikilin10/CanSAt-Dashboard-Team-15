export default function PendingValue({ value, decimals = 1 }) {
  if (value === null || value === undefined || Number.isNaN(value)) {
    return <span className="text-muted">—</span>;
  }
  return <>{Number(value).toFixed(decimals)}</>;
}
