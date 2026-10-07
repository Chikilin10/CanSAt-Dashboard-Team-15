import useTelemetry from "../hooks/useTelemetry";
import StatusBar from "../components/layout/StatusBar";
import CardFactory from "../dashboard/CardFactory";
import { CARD_SPECS } from "../dashboard/cardSpecs";

export default function Dashboard() {
  const { data, last } = useTelemetry();
  return (
    <>
      <StatusBar />
      <div className="row row-cols-1 row-cols-md-2 row-cols-xl-4 g-3">
        {CARD_SPECS.map((spec) => (
          <div className="col" key={spec.id}>
            <CardFactory spec={spec} last={last} history={data} />
          </div>
        ))}
      </div>
    </>
  );
}
