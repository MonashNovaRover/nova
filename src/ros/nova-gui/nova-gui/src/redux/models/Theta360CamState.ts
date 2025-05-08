export enum InteractionMode {
  PAN = "PAN",
  MEASURE = "MEASURE",
}

export interface AngleCoordinate {
  theta: number;
}

export interface Theta360CamState {
  interactionMode: InteractionMode;

  measure: {
    from?: AngleCoordinate;
    to?: AngleCoordinate;
    measuring: boolean;
  };

}