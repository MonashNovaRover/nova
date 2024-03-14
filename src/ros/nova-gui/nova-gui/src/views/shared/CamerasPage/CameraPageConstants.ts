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


export const initialisedFilters: CameraFilters = {
  flipCamera: false,
  invertCamera: false,
  rotation: 0,
  contrast: 100, // In %
  brightness: 100, // in %
};

export enum CameraSerials {
  MAST_ARM_STOW = "mast_arm_stow",
  MAST_DOWN = "mast_down",
  MAST_FORWARD = "mast_forward",
  MAST_BACKWARD = "mast_backward",

  ARM_END_SIDE = "arm_end_side",
  ARM_END_TOP = "arm_end_top",
  ARM_END_FINGER = "arm_end_finger",
  ARM_END_FORWARD = "arm_end_forward",
  ARM_GIMBAL = "arm_gimbal",

  EC_SCRAPER = "ce_scraper",
  EC_FORKLIFT_DOWN = "ce_forklift_down",
  EC_FORKLIFT_FORWARD = "ce_forklift_forward",
  EC_SCRAPER_LEG = "ce_scraper_leg",

  SCIENCE_KILN = "science_kiln",
  SCIENCE_AUGER_BOTTOM = "science_auger_bottom",
}

export const defaultCamFilters : {[key: string] : CameraFilters} = {
  "arm_end_forward": {
    flipCamera: true,
    invertCamera: false,
    rotation: 0,
    contrast: 100,
    brightness: 100,
  },
  "arm_gimbal": {
    flipCamera: false,
    invertCamera: false,
    rotation: 90,
    contrast: 100,
    brightness: 100,
  },
}

export const allCams = [];

const mastCams = [
  CameraSerials.MAST_ARM_STOW,
  CameraSerials.MAST_DOWN,
  CameraSerials.MAST_FORWARD,
  CameraSerials.MAST_BACKWARD,
];

const armCams = [
  CameraSerials.ARM_END_TOP,
  CameraSerials.ARM_END_FINGER,
  CameraSerials.ARM_END_FORWARD,
  CameraSerials.ARM_GIMBAL,
];

const ceCams = [
  CameraSerials.EC_SCRAPER,
  CameraSerials.EC_FORKLIFT_DOWN,
  CameraSerials.EC_FORKLIFT_FORWARD,
  CameraSerials.EC_SCRAPER_LEG,
];


export const post_landing_views: CameraView[] = [
  {
    cameraSerials: [...mastCams, ...armCams],
    viewTitle: "All PL Cams",
  },
  {
    cameraSerials: mastCams,
    viewTitle: "Mast Cams",
  },
  {
    cameraSerials: armCams,
    viewTitle: "Arm Cams",
  },
];

export const excavation_and_construction_views: CameraView[] = [
  {
    cameraSerials: [...mastCams, ...ceCams],
    viewTitle: "All EC Cams",
  },
  {
    cameraSerials: [
      CameraSerials.MAST_ARM_STOW,
      CameraSerials.MAST_DOWN,
      CameraSerials.MAST_FORWARD,
      CameraSerials.MAST_BACKWARD,
    ],
    viewTitle: "Mast Cams",
  },
  {
    cameraSerials: [
      CameraSerials.EC_FORKLIFT_DOWN,
      CameraSerials.EC_FORKLIFT_FORWARD,
      CameraSerials.EC_SCRAPER,
      CameraSerials.EC_SCRAPER_LEG,
    ],
    viewTitle: "CE Cams",
  },
];

export const space_resources_views: CameraView[] = [
  {
    cameraSerials: [
      
    ],
    viewTitle: "All Cameras",
  },
  {
    cameraSerials: [
      CameraSerials.MAST_ARM_STOW,
      CameraSerials.MAST_DOWN,
      CameraSerials.MAST_FORWARD,
      CameraSerials.MAST_BACKWARD,
    ],
    viewTitle: "Mast",
  },
  {
    cameraSerials: [
  
    ],
    viewTitle: "Arm",
  },
];

export const autonomous_views: CameraView[] = [
  {
    cameraSerials: [
      CameraSerials.MAST_ARM_STOW,
      CameraSerials.MAST_DOWN,
      CameraSerials.MAST_FORWARD,
      CameraSerials.MAST_BACKWARD,
    ],
    viewTitle: "All Auto Cams",
  },
  {
    cameraSerials: [
      CameraSerials.MAST_ARM_STOW,
      CameraSerials.MAST_DOWN,
      CameraSerials.MAST_FORWARD,
      CameraSerials.MAST_BACKWARD,
    ],
    viewTitle: "Mast",
  },
];

export const cameraSetup = {
  [ARCCompModes.POST_LANDING]: post_landing_views,
  [ARCCompModes.EXCAVATION_AND_CONSTRUCTION]: excavation_and_construction_views,
  [ARCCompModes.SPACE_RESOURCES]: space_resources_views,
  [ARCCompModes.AUTONOMOUS]: autonomous_views,
};