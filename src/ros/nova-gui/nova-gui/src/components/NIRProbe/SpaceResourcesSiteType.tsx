export enum SpaceResourcesSiteType {
  WATER = 0,
  ILMENITE = 1
}

export interface ISpaceResourcesEntry {
  lightBlank?: number,
  difference: number,
  concentration?: number,
  label: string,
}

export interface ISpaceResourcesFile {
  entries: ISpaceResourcesEntry[],
  type: SpaceResourcesSiteType,
}
