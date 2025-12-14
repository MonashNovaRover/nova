export interface CameraSettings {
  flipCamera: boolean;
  invertCamera: boolean;
  rotation: number;
  contrast: number;
  brightness: number;
}

export interface CameraProfile {
  name: string;
  timestamp: number;
  cameras: {
    [cameraSerial: string]: CameraSettings;
  };
}

export interface CameraProfilesState {
  profiles: {
    [profileName: string]: CameraProfile;
  };
}

export const initialCameraProfilesState: CameraProfilesState = {
  profiles: {},
};
