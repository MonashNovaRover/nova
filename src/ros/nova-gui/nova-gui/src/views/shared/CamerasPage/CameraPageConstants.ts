import { CameraFilters } from "../../../components/CameraComponent/CameraComponent";

export interface CameraView {
  viewTitle: string;
  cameraSerials: string[];
}

export enum ARCCompModes {
  POST_LANDING = "post-landing",
  EXCAVATION_AND_CONSTRUCTION = "excavation-construction",
  SPACE_RESOURCES = "space-resources",
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
  MAST_FISHEYE = "mast_fisheye",
  MAST_DOWN = "mast_down",
  MAST_FORWARD = "mast_forward",
  MAST_BACKWARD = "mast_backward",

  ARM_END_SIDE = "arm_end_side",
  ARM_END_TOP = "arm_end_top",
  ARM_END_FINGER = "arm_end_finger",
  ARM_END_FORWARD = "arm_end_forward",
  ARM_GIMBAL = "arm_gimbal",

  EC_SCRAPER = "ec_scraper",
  EC_FORKLIFT_DOWN = "ec_forklift_down",
  EC_FORKLIFT_FORWARD = "ec_forklift_forward",
  EC_SCRAPER_LEG = "ec_scraper_leg",

  SCIENCE_KILN = "science_kiln",
  SCIENCE_AUGER_BOTTOM = "science_auger_bottom",
  SCIENCE_ANALYSIS_BOTTOM = "science_analysis_bottom",
  SCIENCE_MICROSCOPE = "science_microscope",
}

export const defaultCamFilters: { [key: string]: CameraFilters } = {
  arm_end_forward: {
    flipCamera: true,
    invertCamera: false,
    rotation: 0,
    contrast: 100,
    brightness: 100,
  },
  arm_end_finger: {
    flipCamera: false,
    invertCamera: false,
    rotation: -90,
    contrast: 100,
    brightness: 100,
  },
  "science_kiln": {
    flipCamera: false,
    invertCamera: false,
    rotation: 180,
    contrast: 100,
    brightness: 100,
  }
}

export const allCams = [];

const mastCams = [
  CameraSerials.MAST_FISHEYE,
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

const ecCams = [
  CameraSerials.EC_SCRAPER,
  CameraSerials.EC_FORKLIFT_DOWN,
  CameraSerials.EC_FORKLIFT_FORWARD,
  CameraSerials.EC_SCRAPER_LEG,
];

const scienceCams = [
  CameraSerials.SCIENCE_KILN,
  CameraSerials.SCIENCE_AUGER_BOTTOM,
  CameraSerials.SCIENCE_ANALYSIS_BOTTOM,
  CameraSerials.SCIENCE_MICROSCOPE,
];

export const post_landing_views: CameraView[] = [
  {
    cameraSerials: [...mastCams, ...armCams],
    viewTitle: "All Cams",
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
    cameraSerials: [...mastCams, ...ecCams],
    viewTitle: "All Cams",
  },
  {
    cameraSerials: mastCams,
    viewTitle: "Mast Cams",
  },
  {
    cameraSerials: ecCams,
    viewTitle: "EC Cams",
  },
];

export const space_resources_views: CameraView[] = [
  {
    cameraSerials: [
      CameraSerials.MAST_FISHEYE,
      CameraSerials.MAST_FORWARD,

      CameraSerials.SCIENCE_KILN,
      CameraSerials.SCIENCE_MICROSCOPE,

      CameraSerials.MAST_DOWN,
      CameraSerials.MAST_BACKWARD,

      CameraSerials.SCIENCE_ANALYSIS_BOTTOM,
      CameraSerials.SCIENCE_AUGER_BOTTOM,
    ],
    viewTitle: "All Cams",
  },
  {
    cameraSerials: mastCams,
    viewTitle: "Mast Cams",
  },
  {
    cameraSerials: scienceCams,
    viewTitle: "Science Cams",
  },
];

export const autonomous_views: CameraView[] = [
  {
    cameraSerials: [...mastCams],
    viewTitle: "All Cams",
  },
  {
    cameraSerials: mastCams,
    viewTitle: "Mast Cams",
  },
];

export const cameraSetup = {
  [ARCCompModes.POST_LANDING]: post_landing_views,
  [ARCCompModes.EXCAVATION_AND_CONSTRUCTION]: excavation_and_construction_views,
  [ARCCompModes.SPACE_RESOURCES]: space_resources_views,
  [ARCCompModes.AUTONOMOUS]: autonomous_views,
};
