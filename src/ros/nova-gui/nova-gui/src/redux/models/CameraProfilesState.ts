/**
 * Camera filter settings applied to individual camera feeds
 */
export interface CameraSettings {
  flipCamera: boolean;
  invertCamera: boolean;
  rotation: number; // Rotation angle in degrees (-180 to 180)
  contrast: number; // Contrast level (0 to 200)
  brightness: number; // Brightness level (0 to 200)
}

/**
 * A saved camera profile containing settings for multiple cameras
 */
export interface CameraProfile {
  name: string; // User-defined profile name
  timestamp: number; // Unix timestamp when profile was created
  cameras: {
    [cameraSerial: string]: CameraSettings;
  };
}

/**
 * Redux state for managing saved camera profiles
 */
export interface CameraProfilesState {
  profiles: {
    [profileName: string]: CameraProfile;
  };
  lastLoadedProfile: string | null; // Name of the last loaded profile
}

export const initialCameraProfilesState: CameraProfilesState = {
  profiles: {},
  lastLoadedProfile: null,
};
