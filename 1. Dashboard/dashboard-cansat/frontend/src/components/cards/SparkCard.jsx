import { LineChart, Line, YAxis, ResponsiveContainer } from "recharts";
import CardShell from "./CardShell";
import PendingValue from "../common/PendingValue";

export default function SparkCard({ value, unit, decimals = 1, history = [], dataKey, color = "#198754", ...shell }) {
  const pending = value == null;
  return (
    <CardShell {...shell} pending={pending}>
      <div className="fs-2 fw-bold text-center">
        <PendingValue value={value} decimals={decimals} />
        {!pending && <small className="fs-6 text-muted ms-1">{unit}</small>}
      </div>
      <div style={{ height: 60 }}>
        <ResponsiveContainer width="100%" height="100%">
          <LineChart data={history}>
            <YAxis hide domain={["auto", "auto"]} />
            <Line type="monotone" dataKey={dataKey} stroke={color} strokeWidth={2} dot={false} isAnimationActive={false} />
          </LineChart>
        </ResponsiveContainer>
      </div>
    </CardShell>
  );
}
