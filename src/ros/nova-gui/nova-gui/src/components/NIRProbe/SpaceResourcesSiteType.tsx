import {Droplet, Square, XCircle} from "react-feather";

export enum SpaceResourcesSiteType {
  WATER = 0,
  ILMENITE = 1,
}

export enum NIRProbeReadingType {
  OFF = 0,
  WATER = 1,
  ICE = 2,
}

export const NIRPRobeReadingTypeInfo = [
  {
    type: NIRProbeReadingType.OFF,
    name: "Off",
    icon: <XCircle size={18}/>,
    colour: "default",
  },
  {
    type: NIRProbeReadingType.WATER,
    name: "Water",
    icon: <Droplet size={18}/>,
    colour: "primary",
  },
  {
    type: NIRProbeReadingType.ICE,
    name: "Ice",
    icon: <Square size={18}/>,
    colour: "secondary",
  },
]

export enum XYNames {
  X = "Water",
  Y = "Ice",
  FXY = "Concentration",
}

export interface ISpaceResourcesEntries {
  [NIRProbeReadingType.WATER]: ISpaceResourcesEntry[]
  [NIRProbeReadingType.ICE]: ISpaceResourcesEntry[]
}

export interface ISpaceResourcesEntry {
  data: number,
  type: NIRProbeReadingType,
  label: string,
}
