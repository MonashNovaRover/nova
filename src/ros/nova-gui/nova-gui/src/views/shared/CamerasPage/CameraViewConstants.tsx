import {ProfileOption} from "./CameraProfileConstants.ts";

export interface SerialPreset {
  displayName: string;    // User-facing name (e.g., "Site Analysis Cams")
  serials: string[];      // Array of camera serials
  section?: string;       // Optional section heading (rendered when it changes)
}

export interface SerialPresetGroup {
  groupName: string;
  presets: SerialPreset[];
  mode: "controls" | "toggle";  // controls = Start/Pause/Stop, toggle = single-activate switches
}

export interface CameraViewConfig {
  viewTitle: string;
  cameraSerials: string[];
  cameraPrests?: ProfileOption[];
  serialPresetGroups?: SerialPresetGroup[];
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

export enum CameraSerials {
  WHEEL_TELEMETRY = "wheel_telemetry",
  DRIVE_TELEMETRY = "drive_telemetry",
  DRIVE_CONTROL = "drive_control",
  SITE_SELECT = "site_select",
  SCIENCE_POWER_CYCLE = "power_cycle",
  SCIENCE_COMBINED = "science_combined",
  ARM_TELEMETRY = "arm_telemetry",

  MAST_FISHEYE = "mast_fisheye",
  MAST_BELLY = "mast_belly",
  MAST_FORWARD = "mast_forward",
  MAST_BACKWARD = "mast_backward",
  MAST_ARM_STOW = "mast_arm_stow",

  ARM_END_SIDE = "arm_end_side",
  ARM_END_TOP = "arm_end_top",
  ARM_END_FINGER = "arm_end_finger",
  ARM_END_PERISCOPE = "arm_end_periscope",
  ARM_WRIST = "arm_wrist",

  EC_SCRAPER = "ec_scraper",
  EC_FORKLIFT_DOWN = "ec_forklift_down",
  EC_FORKLIFT_FORWARD = "ec_forklift_forward",
  EC_SCRAPER_LEG = "ec_scraper_leg",

  SCIENCE_KILN_BOOM = "science_kiln_boom",
  SCIENCE_ANALYSIS_ARM_DOWN = "science_analysis_arm_down",
  SCIENCE_MICROSCOPE = "science_microscope",
  SCIENCE_GIMBAL = "science_gimbal_cam",

  ARC_ACTIVATED_NODES = "arc_activated_nodes",

  URC_SCIENCE_UV_VIS = "science_spectroscope",
  URC_SCIENCE_CUVETTE = "science_cuvettes",
  URC_SCIENCE_CACHE_LEFT = "science_cache_left",
  URC_SCIENCE_CACHE_RIGHT = "science_cache_right",
  URC_SCIENCE_BOOM = "science_boom",
  URC_SCIENCE_LITMUS = "science_litmus",
  URC_ACTIVATED_NODES = "urc_activated_nodes",
  URC_SCIENCE_AUGER_DEPTH_SENSORS = "science_auger_depth_sensors",

  AUTO_OAK = "oak-rgb",
  AUTO_BOOTIE = "bootie-rgb",
  AUTO_FORWARD = "auto_forward",
  AUTO_LEFT = "auto_left",
  AUTO_RIGHT = "auto_right",
  AUTO_BEHIND = "auto_behind",
}

export const allCams = [];

const mastCams = [
  CameraSerials.MAST_BELLY,
  CameraSerials.MAST_FORWARD,
  CameraSerials.MAST_BACKWARD,
  CameraSerials.MAST_ARM_STOW,
];

const armCams = [
  CameraSerials.ARM_END_TOP,
  CameraSerials.ARM_END_FINGER,
  CameraSerials.ARM_END_PERISCOPE,
  CameraSerials.ARM_END_SIDE,
  CameraSerials.ARM_WRIST,
];

const urcArmCams = [
  CameraSerials.MAST_ARM_STOW,
  CameraSerials.MAST_FORWARD,
  CameraSerials.ARM_END_TOP,
  CameraSerials.ARM_END_FINGER,
  CameraSerials.MAST_BACKWARD,
  CameraSerials.MAST_BELLY,
  CameraSerials.ARM_END_PERISCOPE,
  CameraSerials.ARM_END_SIDE,
  CameraSerials.ARM_WRIST,
]

const ecCams = [
  CameraSerials.EC_SCRAPER,
  CameraSerials.EC_FORKLIFT_DOWN,
  CameraSerials.EC_FORKLIFT_FORWARD,
  CameraSerials.EC_SCRAPER_LEG,
];

const arcScienceCams = [
  CameraSerials.SCIENCE_KILN_BOOM,
  CameraSerials.SCIENCE_GIMBAL,
  CameraSerials.SCIENCE_ANALYSIS_ARM_DOWN,
  CameraSerials.SCIENCE_MICROSCOPE,
];

const urcScienceCams = [
  [
    CameraSerials.SCIENCE_GIMBAL,
    CameraSerials.URC_SCIENCE_CACHE_LEFT,
    CameraSerials.URC_SCIENCE_CACHE_RIGHT,
    CameraSerials.URC_SCIENCE_BOOM,
    CameraSerials.SCIENCE_MICROSCOPE,
  ],
  [
    CameraSerials.SCIENCE_GIMBAL,
    CameraSerials.URC_SCIENCE_CUVETTE,
    CameraSerials.URC_SCIENCE_UV_VIS,
    CameraSerials.URC_SCIENCE_LITMUS,
  ],
]

const urcScienceLocationPresets: SerialPreset[] = [
  {
    displayName: "Mast Cams",
    serials: [
      CameraSerials.MAST_FORWARD,
      CameraSerials.MAST_BACKWARD,
      CameraSerials.MAST_ARM_STOW,
    ],
  },
  {
    displayName: "Payload Cams",
    serials: [
      CameraSerials.URC_SCIENCE_BOOM,
      CameraSerials.URC_SCIENCE_CACHE_LEFT,
      CameraSerials.URC_SCIENCE_CACHE_RIGHT,
    ],
  },
  {
    displayName: "Analysis Cams",
    serials: [
      CameraSerials.SCIENCE_MICROSCOPE,
      CameraSerials.SCIENCE_GIMBAL,
    ],
  },
  {
    displayName: "Internal Cams",
    serials: [
      CameraSerials.URC_SCIENCE_CUVETTE,
      CameraSerials.URC_SCIENCE_LITMUS,
    ],
  },
]

const urcScienceTaskPresets: SerialPreset[] = [
  {
    section: "Site",
    displayName: "Surveying",
    serials: [
      CameraSerials.MAST_FORWARD,
      CameraSerials.MAST_BACKWARD,
      CameraSerials.MAST_ARM_STOW,
      CameraSerials.SCIENCE_GIMBAL,
      CameraSerials.SCIENCE_MICROSCOPE,
      CameraSerials.URC_SCIENCE_BOOM,
    ],
  },
  {
    section: "Site",
    displayName: "Sampling",
    serials: [
      CameraSerials.SCIENCE_GIMBAL,
      CameraSerials.URC_SCIENCE_CACHE_LEFT,
      CameraSerials.URC_SCIENCE_CACHE_RIGHT,
      CameraSerials.URC_SCIENCE_BOOM,
    ],
  },
  {
    section: "Pumping",
    displayName: "Stage 1",
    serials: [
      CameraSerials.SCIENCE_GIMBAL,
      CameraSerials.URC_SCIENCE_CACHE_LEFT,
      CameraSerials.URC_SCIENCE_CACHE_RIGHT,
      CameraSerials.URC_SCIENCE_BOOM,
      CameraSerials.URC_SCIENCE_LITMUS,
    ],
  },
  {
    section: "Pumping",
    displayName: "Stage 2",
    serials: [
      CameraSerials.URC_SCIENCE_CUVETTE,
      CameraSerials.URC_SCIENCE_LITMUS,
    ],
  },
]

const urcSciencePresetGroups: SerialPresetGroup[] = [
  { groupName: "Location", presets: urcScienceLocationPresets, mode: "controls" },
  { groupName: "Task", presets: urcScienceTaskPresets, mode: "toggle" },
]

const driveCams = [
  CameraSerials.WHEEL_TELEMETRY,
  CameraSerials.DRIVE_TELEMETRY,
  CameraSerials.DRIVE_CONTROL,
]

const autoCams = [
  CameraSerials.AUTO_LEFT,
  CameraSerials.AUTO_FORWARD,
  CameraSerials.AUTO_RIGHT,
  CameraSerials.AUTO_BEHIND,
]

export const post_landing_views: CameraViewConfig[] = [
  {
    cameraSerials: [...mastCams, ...armCams, ...driveCams],
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

export const urc_equipment_servicing_views: CameraViewConfig[] = [
  {
    cameraSerials: [...urcArmCams.slice(0,8), ...driveCams, urcArmCams[urcArmCams.length - 1]],
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

export const urc_delivery_views: CameraViewConfig[] = [
  {
    cameraSerials: [...urcArmCams.slice(0,3), urcArmCams[7], ...urcArmCams.slice(4,6), urcArmCams[6], urcArmCams[8], ...driveCams.slice(0, 2), CameraSerials.ARM_TELEMETRY, urcArmCams[3], CameraSerials.DRIVE_CONTROL],
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

export const excavation_and_construction_views: CameraViewConfig[] = [
  {
    cameraSerials: [...mastCams, ...ecCams, ...driveCams],
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

export const space_resources_views: CameraViewConfig[] = [
  {
    cameraSerials: [...mastCams, ...arcScienceCams, ...driveCams, CameraSerials.SITE_SELECT, CameraSerials.SCIENCE_POWER_CYCLE, CameraSerials.ARC_ACTIVATED_NODES],
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

export const autonomous_views: CameraViewConfig[] = [
  {
    cameraSerials: [...mastCams, ...driveCams],
    viewTitle: "All Cams",
  },
  {
    cameraSerials: mastCams,
    viewTitle: "Mast Cams",
  },
];

export const urc_autonomous_views: CameraViewConfig[] = [
  {
    cameraSerials: [...autoCams, ...driveCams],
    viewTitle: "All Cams",
  },
  {
    cameraSerials: autoCams,
    viewTitle: "Auto Cams",
  },
];

export const urc_science_views: CameraViewConfig[] = [
  {
    cameraSerials: [...mastCams.slice(1, 4), ...urcScienceCams[0], ...urcScienceCams[1],  ...driveCams.slice(0, 2), CameraSerials.URC_ACTIVATED_NODES, CameraSerials.URC_SCIENCE_AUGER_DEPTH_SENSORS, CameraSerials.DRIVE_CONTROL],
    viewTitle: "All Cams",
    serialPresetGroups: urcSciencePresetGroups,
  },
  {
    cameraSerials: [CameraSerials.MAST_FORWARD, CameraSerials.MAST_ARM_STOW, ...urcScienceCams[0], CameraSerials.URC_SCIENCE_LITMUS, ...driveCams.slice(0, 2), CameraSerials.URC_ACTIVATED_NODES, CameraSerials.URC_SCIENCE_AUGER_DEPTH_SENSORS, CameraSerials.DRIVE_CONTROL],
    viewTitle: "Site Analysis",
    serialPresetGroups: urcSciencePresetGroups,
  },
  {
    cameraSerials: [...mastCams.slice(3, 4), CameraSerials.URC_SCIENCE_CACHE_LEFT, CameraSerials.URC_SCIENCE_CACHE_RIGHT, ...urcScienceCams[1],  ...driveCams.slice(0, 2), CameraSerials.URC_ACTIVATED_NODES, CameraSerials.DRIVE_CONTROL],
    viewTitle: "Vis Spec",
    serialPresetGroups: urcSciencePresetGroups,
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
