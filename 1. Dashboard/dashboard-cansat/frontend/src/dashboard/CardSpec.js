// Describe una tarjeta: qué campo muestra, de qué tipo es y cómo se evalúa su estado.
export default class CardSpec {
  constructor({ id, type, field, statusFn = null, ...props }) {
    this.id = id;
    this.type = type; // "number" | "spark" | "progress" | "gauge"
    this.field = field;
    this.statusFn = statusFn;
    this.props = props;
  }

  evaluate(last, history) {
    return {
      ...this.props,
      dataKey: this.field,
      value: last?.[this.field],
      history,
      ...this.statusFn?.(last),
    };
  }
}
