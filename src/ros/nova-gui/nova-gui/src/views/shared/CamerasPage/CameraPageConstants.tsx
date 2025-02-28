import { CameraFilters } from "../../../components/CameraComponent/CameraComponent";
import {ReactNode} from "react";
import {Minus} from "react-feather";

export interface CameraView {
  viewTitle: string;
  cameraSerials: string[];
}

export enum ARCCompModes {
  ARC_POST_LANDING = "post-landing",
  ARC_EXCAVATION_AND_CONSTRUCTION = "excavation-construction",
  ARC_SPACE_RESOURCES = "space-resources",
  ARC_AUTONOMOUS = "autonomous",
}

export enum URCCompModes {
  URC_EQUIPMENT_SERVICING = "equipment-servicing",
  URC_DELIVERY = "delivery",
  URC_SCIENCE = "science",
  URC_AUTONOMOUS = "autonomous-navigation",
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
  MAST_ARM_STOW = "mast_arm_stow",

  ARM_END_SIDE = "arm_end_side",
  ARM_END_TOP = "arm_end_top",
  ARM_END_FINGER = "arm_end_finger",
  ARM_END_PERISCOPE = "arm_end_periscope",
  ARM_GIMBAL = "arm_gimbal",

  EC_SCRAPER = "ec_scraper",
  EC_FORKLIFT_DOWN = "ec_forklift_down",
  EC_FORKLIFT_FORWARD = "ec_forklift_forward",
  EC_SCRAPER_LEG = "ec_scraper_leg",

  SCIENCE_KILN = "science_kiln",
  SCIENCE_AUGER_BOTTOM = "science_auger_bottom",
  SCIENCE_ANALYSIS_BOTTOM = "science_analysis_bottom",
  SCIENCE_MICROSCOPE = "science_microscope",

  URC_SCIENCE_UV_VIS = "science_spectroscope",
  URC_SCIENCE_CUVETTE = "science_cuvettes",
  URC_SCIENCE_PAYLOAD_FRONT = "science_payload_front",
  URC_SCIENCE_PAYLOAD_DOWN = "science_payload_down",
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
    rotation: 0,
    contrast: 100,
    brightness: 100,
  },
  mast_down: {
    flipCamera: false,
    invertCamera: false,
    rotation: 0,
    contrast: 100,
    brightness: 100,
  },
  science_analysis_bottom: {
    flipCamera: false,
    invertCamera: false,
    rotation: -90,
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
  CameraSerials.MAST_ARM_STOW,
];

const armCams = [
  CameraSerials.ARM_END_TOP,
  CameraSerials.ARM_END_FINGER,
  CameraSerials.ARM_END_PERISCOPE,
  CameraSerials.ARM_GIMBAL,
  CameraSerials.ARM_END_SIDE,
];

const ecCams = [
  CameraSerials.EC_SCRAPER,
  CameraSerials.EC_FORKLIFT_DOWN,
  CameraSerials.EC_FORKLIFT_FORWARD,
  CameraSerials.EC_SCRAPER_LEG,
];

const arcScienceCams = [
  CameraSerials.SCIENCE_KILN,
  CameraSerials.SCIENCE_AUGER_BOTTOM,
  CameraSerials.SCIENCE_ANALYSIS_BOTTOM,
  CameraSerials.SCIENCE_MICROSCOPE,
];

const urcScienceCams = [
  CameraSerials.URC_SCIENCE_CUVETTE,
  CameraSerials.SCIENCE_MICROSCOPE,
  CameraSerials.URC_SCIENCE_PAYLOAD_DOWN,
  CameraSerials.URC_SCIENCE_PAYLOAD_FRONT,
  CameraSerials.URC_SCIENCE_UV_VIS,
]

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

export const urc_equipment_servicing_views: CameraView[] = [
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

export const urc_delivery_views: CameraView[] = [
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

      CameraSerials.SCIENCE_AUGER_BOTTOM,
      CameraSerials.SCIENCE_KILN,

      CameraSerials.MAST_DOWN,
      CameraSerials.MAST_BACKWARD,

      CameraSerials.SCIENCE_ANALYSIS_BOTTOM,
      
    ],
    viewTitle: "All Cams",
  },
  {
    cameraSerials: mastCams,
    viewTitle: "Mast Cams",
  },
  {
    cameraSerials: arcScienceCams,
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

export const urc_autonomous_views: CameraView[] = [
  {
    cameraSerials: [...mastCams],
    viewTitle: "All Cams",
  },
  {
    cameraSerials: mastCams,
    viewTitle: "Mast Cams",
  },
];

export const urc_science_views: CameraView[] = [
  {
    cameraSerials: [...mastCams, ...urcScienceCams],
    viewTitle: "All Cams",
  },
  {
    cameraSerials:  mastCams,
    viewTitle: "Mast Cams",
  },
  {
    cameraSerials: urcScienceCams,
    viewTitle: "Science Cams"
  }
]

export const arcCameraSetup = {
  [ARCCompModes.ARC_POST_LANDING]: post_landing_views,
  [ARCCompModes.ARC_EXCAVATION_AND_CONSTRUCTION]: excavation_and_construction_views,
  [ARCCompModes.ARC_SPACE_RESOURCES]: space_resources_views,
  [ARCCompModes.ARC_AUTONOMOUS]: autonomous_views,
 
};

export const urcCameraSetup = {
  [URCCompModes.URC_EQUIPMENT_SERVICING]: urc_equipment_servicing_views,
  [URCCompModes.URC_DELIVERY]: urc_delivery_views,
  [URCCompModes.URC_SCIENCE]: urc_science_views,
  [URCCompModes.URC_AUTONOMOUS]: urc_autonomous_views,
}

export const defaultCameraOverlays: { [k: string]: ReactNode } = {
  [CameraSerials.ARM_END_TOP]: <Minus className="self-center" color="black"/>,
  [CameraSerials.ARM_END_FINGER]: <Minus className="self-center" color="black"/>,
  [CameraSerials.ARM_END_PERISCOPE]: <Minus className="self-center" color="black"/>,
  [CameraSerials.ARM_GIMBAL]: <Minus className="self-center" color="black"/>,
  [CameraSerials.ARM_END_SIDE]: <Minus className="self-center" color="black"/>,
}