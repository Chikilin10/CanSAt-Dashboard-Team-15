export default {
  dataSource: import.meta.env.VITE_DATA_SOURCE ?? "mock", // "mock" | "ws"
  wsUrl: import.meta.env.VITE_WS_URL ?? "ws://localhost:8080",
  bufferSize: 300,
  mockIntervalMs: 250,
};
