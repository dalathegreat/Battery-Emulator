export interface EventRow {
  type: string;
  severity: string;
  age_ms: number;
  count: number;
  data: number;
  message: string;
}

export interface EventsResponse {
  events: EventRow[];
}
