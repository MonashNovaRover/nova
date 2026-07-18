import {ArrowDownLeft, ArrowDownRight, Pocket, Search} from "react-feather";
import {ReactNode} from "react";

export interface ActivatedNodeConfig {
  name: string
  displayName: string
  icon: ReactNode
}

export const URCActivatedNodeConfig: ActivatedNodeConfig[] = [
  {
    name: "auger_left",
    displayName: "Left Auger",
    icon: <ArrowDownLeft/>,
  },
  {
    name: "auger_right",
    displayName: "Right Auger",
    icon: <ArrowDownRight/>,
  },
  {
    name: "cbeam",
    displayName: "C Beam",
    icon: <Pocket/>,
  },
  {
    name: "analysis_arm",
    displayName: "Analysis Arm",
    icon: <Search/>,
  },
]

export const ARCActivatedNodeConfig: ActivatedNodeConfig[] = [
  {
    name: "cbeam",
    displayName: "C Beam",
    icon: <Pocket/>,
  },
  {
    name: "analysis_arm",
    displayName: "Analysis Arm",
    icon: <Search/>,
  },
]
