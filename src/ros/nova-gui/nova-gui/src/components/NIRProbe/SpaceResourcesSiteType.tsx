export enum SpaceResourcesSiteType {
  WATER = 0,
  ILMENITE = 1
}

export interface ISpaceResourcesEntry {
  x: number,
  y: number,
  fxy?: number,
  label: string,
}

export interface ISpaceResourcesFile {
  entries: ISpaceResourcesEntry[],
}
