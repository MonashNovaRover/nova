import {CameraFilters} from "../../../components/cameras/CameraComponent/CameraComponent.tsx";

export const initialisedFilters: CameraFilters = {
  flipCamera: false,
  invertCamera: false,
  rotation: 0,
  contrast: 100, // In %
  brightness: 100, // in %
};

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
  arm_end_periscope: {
    flipCamera: true,
    invertCamera: false,
    rotation: 0,
    contrast: 100,
    brightness: 100,
  },
  "science_kiln": {
    flipCamera: false,
    invertCamera: false,
    rotation: 180,
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
  mast_forward: {
    flipCamera: false,
    invertCamera: false,
    rotation: 90,
    contrast: 100,
    brightness: 100,
  },
  science_analysis_bottom: {
    flipCamera: false,
    invertCamera: false,
    rotation: -90,
    contrast: 100,
    brightness: 100,
  },
  science_gimbal: {
    flipCamera: false,
    invertCamera: false,
    rotation: 0,
    contrast: 100,
    brightness: 100,
  }
}

