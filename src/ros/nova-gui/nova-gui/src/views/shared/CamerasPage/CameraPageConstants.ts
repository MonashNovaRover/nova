import { CameraFilters } from "../../../components/CameraComponent/CameraComponent";

export interface CameraSectionParams {
  sectionTitle: string;
  cameraSerials: string[];
}

export const XTREME_DELIVERY_CAMS: string[] = [
  "mast_arm_stow",
  "mast_down",
  "mast_forward",
  "mast_backward",
];

export const AUTO_CAMS: string[] = [
  "mast_arm_stow",
  "mast_down",
  "mast_forward",
  "mast_backward",
];

export const SCI_CAMS: string[] = ["camera5", "camera6"];

export const EQUIPMENT_SERVICE: string[] = ["camera7", "camera8"];

export const cameraSections: CameraSectionParams[] = [
  {
    sectionTitle: "Extreme Delivery",
    cameraSerials: XTREME_DELIVERY_CAMS,
  },
  {
    sectionTitle: "Autonomous",
    cameraSerials: AUTO_CAMS,
  },
  {
    sectionTitle: "Science",
    cameraSerials: SCI_CAMS,
  },
  {
    sectionTitle: "Equipment Servicing",
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
};
