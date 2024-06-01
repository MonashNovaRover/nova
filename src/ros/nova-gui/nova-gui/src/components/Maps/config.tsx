// Config for Cartographer

// This is the Name that comes with the MBTiles File Itself
export enum MapTile {
  Hanksville = "Hanksville",
  MDRS = "MDRS_Hi_Res",
}

export const MAP_BOUNDS:  {  [key in MapTile]: [number, number, number, number] }= {
  [MapTile.Hanksville]: [-110.747, 38.3092, -110.626, 38.3991],
  [MapTile.MDRS]: [-110.809, 38.3917, -110.765, 38.4177],
};




