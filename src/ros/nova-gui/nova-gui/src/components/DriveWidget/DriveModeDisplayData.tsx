import { ReactElement } from "react";
import { Maximize2, Meh, RefreshCw, Truck } from "react-feather";

// Data required for displaying a drive mode in the GUI
export interface IDriveModeDisplayData {
  name: string,
  icon: ReactElement,
  keybind?: string,
}

// Data for displaying the various drive modes in the DriveWidget
export const driveModes : IDriveModeDisplayData[] = [
  { 
    name: "Tank", 
    icon: <Truck className="w-2 h-2"/>, 
    keybind: "A"
  } as IDriveModeDisplayData,
  { 
    name: "Strafe", 
    icon: <Maximize2 className="StrafeDriveModeIcon"/>, 
    keybind: "RB" 
  } as IDriveModeDisplayData,
  { 
    name: "Pivot", 
    icon: <RefreshCw className="PivotDriveModeIcon"/>, 
    keybind: "X" 
  } as IDriveModeDisplayData,
  { 
    name: "Autonomous", 
    icon: <Meh className=""/>, 
    keybind: "Y" 
  } as IDriveModeDisplayData,
];