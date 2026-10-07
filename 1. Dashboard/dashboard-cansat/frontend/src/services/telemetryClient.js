import config from "../config";
import MockSource from "./mockSource";
import WsSource from "./wsSource";

// Fábrica: decide la fuente según VITE_DATA_SOURCE.
export default function createSource() {
  return config.dataSource === "ws" ? new WsSource(config.wsUrl) : new MockSource();
}
