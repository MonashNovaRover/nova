import { CameraFilters } from "../../../components/CameraComponent/CameraComponent";

export interface CameraView {
  viewTitle: string;
  cameraSerials: string[];
}

export enum ARCCompModes {
  POST_LANDING = "post_landing",
  EXCAVATION_AND_CONSTRUCTION = "excavation_and_construction",
  SPACE_RESOURCES = "space_resources",
  AUTONOMOUS = "autonomous",
}


export const initialFilters: CameraFilters = {
  flipCamera: false,
  invertCamera: false,
  rotation: 0,
  contrast: 100, // In %
  brightness: 100, // in %
};

export const allCams = [
  "mast_arm_stow",
  "mast_down",
  "mast_forward",
  "mast_backward",
  "arm_end_side",
  "arm_end_top",
  "arm_end_finger",
  "arm_end_forward",
  "arm_gimbal"
];

export const post_landing_views: CameraView[] = [
  {
    cameraSerials: [
      "mast_arm_stow",
      "mast_down",
      "mast_forward",
      "mast_backward",
      "arm_end_side",
      "arm_end_top",
      "arm_end_finger",
      "arm_end_forward",
      "arm_gimbal"
    ],
    viewTitle: "All Cameras",
  },
  {
    cameraSerials: [
      "mast_arm_stow",
      "mast_down",
      "mast_forward",
      "mast_backward",
    ],
    viewTitle: "Mast",
  },
  {
    cameraSerials: [
      "arm_end_side",
      "arm_end_top",
      "arm_end_finger",
      "arm_end_forward",
      "arm_gimbal"
    ],
    viewTitle: "Arm",
  },
];

export const excavation_and_construction_views: CameraView[] = [
  {
    cameraSerials: [
      "mast_arm_stow",
      "mast_down",
      "mast_forward",
      "mast_backward",
      "arm_end_side",
      "arm_end_top",
      "arm_end_finger",
      "arm_end_forward",
    ],
    viewTitle: "All Cameras",
  },
  {
    cameraSerials: [
      "mast_arm_stow",
      "mast_down",
      "mast_forward",
      "mast_backward",
    ],
    viewTitle: "Mast",
  },
  {
    cameraSerials: [
      "arm_end_side",
      "arm_end_top",
      "arm_end_finger",
      "arm_end_forward",
    ],
    viewTitle: "Arm",
  },
];

export const space_resources_views: CameraView[] = [
  {
    cameraSerials: [
      "mast_arm_stow",
      "mast_down",
      "mast_forward",
      "mast_backward",
      "arm_end_side",
      "arm_end_top",
      "arm_end_finger",
      "arm_end_forward",
    ],
    viewTitle: "All Cameras",
  },
  {
    cameraSerials: [
      "mast_arm_stow",
      "mast_down",
      "mast_forward",
      "mast_backward",
    ],
    viewTitle: "Mast",
  },
  {
    cameraSerials: [
      "arm_end_side",
      "arm_end_top",
      "arm_end_finger",
      "arm_end_forward",
    ],
    viewTitle: "Arm",
  },
];

export const autonomous_views: CameraView[] = [
  {
    cameraSerials: [
      "mast_arm_stow",
      "mast_down",
      "mast_forward",
      "mast_backward",
      "arm_end_side",
      "arm_end_top",
      "arm_end_finger",
      "arm_end_forward",
    ],
    viewTitle: "All Cameras",
  },
  {
    cameraSerials: [
      "mast_arm_stow",
      "mast_down",
      "mast_forward",
      "mast_backward",
    ],
    viewTitle: "Mast",
  },
  {
    cameraSerials: [
      "arm_end_side",
      "arm_end_top",
      "arm_end_finger",
      "arm_end_forward",
    ],
    viewTitle: "Arm",
  },
];

export const cameraSetup = {
  [ARCCompModes.POST_LANDING]: post_landing_views,
  [ARCCompModes.EXCAVATION_AND_CONSTRUCTION]: excavation_and_construction_views,
  [ARCCompModes.SPACE_RESOURCES]: space_resources_views,
  [ARCCompModes.AUTONOMOUS]: autonomous_views,
};
