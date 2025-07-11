import {DetailedReactHTMLElement} from "react";
import {Maximize2, RefreshCw} from "react-feather";
import {Image} from "@nextui-org/react";
import Tank from "../../../assets/tank-icon.svg";

// Enum to assign meaning to IRosDriveInterfacesDriveInfo.drive_mode values
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
    icon: <Image src={Tank} className="w-5 h-5"/>,
    keybind: "Y",
    driveMode: DriveMode.TANK
  } as IDriveModeDisplayData,
  { 
    name: "Strafe", 
    icon: <Maximize2 className="StrafeDriveModeIcon"/>, 
    keybind: "LB",
    driveMode: DriveMode.STRAFE
  } as IDriveModeDisplayData,
  { 
    name: "Pivot", 
    icon: <RefreshCw className="PivotDriveModeIcon"/>, 
    keybind: "RB",
    driveMode: DriveMode.PIVOT
  } as IDriveModeDisplayData,
];

