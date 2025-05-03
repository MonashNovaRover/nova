import {ArrowDownLeft, ArrowDownRight, Pocket, Search} from "react-feather";
import {ReactNode} from "react";

export interface ActivatedNodeConfig {
  name: string
  displayName: string
  icon: ReactNode
}

export const URCActivatedNodeConfig: ActivatedNodeConfig[] = [
  {
    name: "Auger1",
    displayName: "Left Auger",
    icon: <ArrowDownLeft/>,
  },
  {
    name: "Auger2",
    displayName: "Right Auger",
    icon: <ArrowDownRight/>,
  },
  {
    name: "CBeam",
    displayName: "C Beam",
    icon: <Pocket/>,
  },
  {
    name: "AnalysisArm",
    displayName: "Analysis Arm",
    icon: <Search/>,
  },
]
