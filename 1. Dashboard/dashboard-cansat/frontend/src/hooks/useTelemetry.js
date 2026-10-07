import { useContext, useSyncExternalStore } from "react";
import { TelemetryContext } from "../context/TelemetryContext";

export function useTelemetryStore() {
  return useContext(TelemetryContext);
}

// Devuelve { data, last, status, received, firstPacket }
export default function useTelemetry() {
  const store = useTelemetryStore();
  return useSyncExternalStore(store.subscribe, store.getSnapshot);
}