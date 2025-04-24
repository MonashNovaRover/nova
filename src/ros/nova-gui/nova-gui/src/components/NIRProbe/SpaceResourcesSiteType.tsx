import {Circle, Droplet, Icon, Square, XCircle} from "react-feather";

export enum SpaceResourcesSiteType {
  WATER = 0,
  ILMENITE = 1,
}

export enum NIRProbeReadingType {
  OFF = 0,
  PD1 = 1,
  PD2 = 2,
}

export interface NIRProbeReadingTypeInfo {
  type: NIRProbeReadingType
  name: string
  icon: Icon
  colour: string
}

export const ARCNIRPRobeReadingTypeInfo: NIRProbeReadingTypeInfo[] = [
  {
    type: NIRProbeReadingType.OFF,
    name: "Off",
    icon: XCircle,
    colour: "default",
  },
  {
    type: NIRProbeReadingType.PD1,
    name: "Water",
    icon: Droplet,
    colour: "primary",
  },
  {
    type: NIRProbeReadingType.PD2,
    name: "Ice",
    icon: Square,
    colour: "secondary",
  },
]

export const URCNIRPRobeReadingTypeInfo: NIRProbeReadingTypeInfo[] = [
  {
    type: NIRProbeReadingType.OFF,
    name: "Off",
    icon: XCircle,
    colour: "default",
  },
  {
    type: NIRProbeReadingType.PD1,
    name: "PD1",
    icon: Circle,
    colour: "primary",
  },
  {
    type: NIRProbeReadingType.PD2,
    name: "PD2",
    icon: Square,
    colour: "secondary",
  },
]

export enum XYNames {
  X = "Water",
  Y = "Ice",
  FXY = "Concentration",
}

export interface ISpaceResourcesEntries {
  [NIRProbeReadingType.PD1]: ISpaceResourcesEntry[]
  [NIRProbeReadingType.PD2]: ISpaceResourcesEntry[]
}

export interface ISpaceResourcesEntry {
  data: number,
  type: NIRProbeReadingType,
  label: string,
}
