import { useEffect, useState } from "react";
import { TelemetryContext } from "./TelemetryContext";
import TelemetryStore from "../services/TelemetryStore";
import createSource from "../services/telemetryClient";
import config from "../config";

export default function TelemetryProvider({ children }) {
  const [store] = useState(() => new TelemetryStore(createSource(), config.bufferSize));

  useEffect(() => {
    store.start();
    return () => store.stop();
  }, [store]);

  return <TelemetryContext.Provider value={store}>{children}</TelemetryContext.Provider>;
}
