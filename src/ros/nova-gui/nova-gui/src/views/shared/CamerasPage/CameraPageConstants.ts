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

export const XTREME_DELIVERY_CAMS: string[] = [
  "mast_arm_stow",
  "mast_down",
  "mast_forward",
  "mast_backward",
  "arm_end_side",
  "arm_end_top",
  "arm_end_finger",
  "arm_end_forward",
];

export const AUTO_CAMS: string[] = [
  "mast_arm_stow",
  "mast_down",
  "mast_forward",
  "mast_backward",
];

export const SCI_CAMS: string[] = ["camera5", "camera6"];

export const EQUIPMENT_SERVICE: string[] = ["camera7", "camera8"];

export const cameraSections: CameraView[] = [
  {
    viewTitle: "Extreme Delivery",
    cameraSerials: XTREME_DELIVERY_CAMS,
  },
  {
    viewTitle: "Autonomous",
    cameraSerials: AUTO_CAMS,
  },
  {
    viewTitle: "Science",
    cameraSerials: SCI_CAMS,
  },
  {
    viewTitle: "Equipment Servicing",
    cameraSerials: EQUIPMENT_SERVICE,
  },
];

export const allCams = [
  ...XTREME_DELIVERY_CAMS,
  ...AUTO_CAMS,
  ...SCI_CAMS,
  ...EQUIPMENT_SERVICE,
];

export const initialFilters: CameraFilters = {
  flipCamera: false,
  invertCamera: false,
  rotation: 0,
  contrast: 100, // In %
  brightness: 100, // in %
};

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
