// Config for Cartographer

// This is the Name that comes with the MBTiles File Itself
export enum MapTile {
  Hanksville = "Hanksville",
  MDRS = "MDRS_Hi_Res",
  Monash = "MonashClayton",
  PolicePaddocks = "PolicePaddocks",
  Mordiallic = "MordiallocHorseBeach",
  DeliveryHQ = "Delivery_HQ",
  DeliveryLQ = "Delivery_LQ",
  ESAHQ = "ESA_HQ",
  ESALQ = "ESA_LQ",
}

export const MAP_BOUNDS:  {  [key in MapTile]: [number, number, number, number] }= {
  [MapTile.Hanksville]: [-110.747, 38.3092, -110.626, 38.3991],
  [MapTile.MDRS]: [-110.809, 38.3917, -110.765, 38.4177],
  [MapTile.Monash]: [145.124,-37.9199,145.148,-37.9034],
  [MapTile.PolicePaddocks]: [145.223,-37.9713,145.268,-37.9387],
  [MapTile.Mordiallic]: [145.077,-38.0106,145.089,-38.0019],
  [MapTile.DeliveryHQ]: [-110.793,38.4059,-110.786,38.4135],
  [MapTile.DeliveryLQ]: [-110.797,38.4,-110.781,38.416],
  [MapTile.ESAHQ]: [-110.797,38.4135,-110.778,38.4264],
  [MapTile.ESALQ]: [-110.802,38.398,-110.772,38.4311],
};
