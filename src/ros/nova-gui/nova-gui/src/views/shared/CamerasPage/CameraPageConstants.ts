export interface CameraSectionParams {
  sectionTitle: string;
  cameraSerials: string[];
}

export const XTREME_DELIVERY_CAMS: string[] = ["camera1"];

export const AUTO_CAMS: string[] = [];

export const SCI_CAMS: string[] = [];

export const EQUIPMENT_SERVICE: string[] = [];

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
