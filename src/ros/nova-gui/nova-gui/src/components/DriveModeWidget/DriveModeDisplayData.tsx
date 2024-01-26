import {DetailedReactHTMLElement} from "react";
import {Maximize2, RefreshCw, Truck} from "react-feather";

// Enum to assign meaning to IRosCoreDriveInfo.drive_mode values
export enum DriveMode {
  PIVOT = 1,
  STRAFE = 2,
  TANK = 3
}

// Data required for displaying a drive mode in the GUI
export interface IDriveModeDisplayData {
  name: string,
  shortName?: string,
  icon: DetailedReactHTMLElement<{className: string}, HTMLElement>,
  keybind?: string,
  driveMode: DriveMode
}

// Data for displaying the various drive modes in the DriveModeWidget
export const driveModes : IDriveModeDisplayData[] = [
  { 
    name: "Tank", 
    icon: <Truck className="w-2 h-2"/>, 
    keybind: "A",
    driveMode: DriveMode.TANK
  } as IDriveModeDisplayData,
  { 
    name: "Strafe", 
    icon: <Maximize2 className="StrafeDriveModeIcon"/>, 
    keybind: "RB",
    driveMode: DriveMode.STRAFE
  } as IDriveModeDisplayData,
  { 
    name: "Pivot", 
    icon: <RefreshCw className="PivotDriveModeIcon"/>, 
    keybind: "X",
    driveMode: DriveMode.PIVOT
  } as IDriveModeDisplayData,
];

