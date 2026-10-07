import TelemetryProvider from "./context/TelemetryProvider";
import Navbar from "./components/layout/Navbar";
import Dashboard from "./pages/Dashboard";

export default function App() {
  return (
    <TelemetryProvider>
      <Navbar />
      <div className="container-fluid py-3">
        <Dashboard />
      </div>
    </TelemetryProvider>
  );
}