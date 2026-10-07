import NumberCard from "../components/cards/NumberCard";
import SparkCard from "../components/cards/SparkCard";
import ProgressCard from "../components/cards/ProgressCard";
import GaugeCard from "../components/cards/GaugeCard";

const REGISTRY = { number: NumberCard, spark: SparkCard, progress: ProgressCard, gauge: GaugeCard };

// Fábrica: convierte un CardSpec en su componente.
export default function CardFactory({ spec, last, history }) {
  const Card = REGISTRY[spec.type];
  return Card ? <Card {...spec.evaluate(last, history)} /> : null;
}
