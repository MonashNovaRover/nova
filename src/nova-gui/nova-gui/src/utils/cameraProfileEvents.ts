// Custom event system for camera profile save/load operations

import { CameraSettings } from "../redux/models/CameraProfilesState";

export const CameraProfileEvents = {
  SAVE_PROFILE: 'camera:save-profile',
  LOAD_PROFILE: 'camera:load-profile',
  FILTERS_READY: 'camera:filters-ready',
} as const;

// Emit save profile event
export const emitSaveProfileEvent = (profileName: string) => {
  const event = new CustomEvent(CameraProfileEvents.SAVE_PROFILE, {
    detail: { profileName },
  });
  window.dispatchEvent(event);
};

// Emit load profile event
export const emitLoadProfileEvent = (profileName: string) => {
  const event = new CustomEvent(CameraProfileEvents.LOAD_PROFILE, {
    detail: { profileName },
  });
  window.dispatchEvent(event);
};

// Emit camera filters ready event
export const emitCameraFiltersReadyEvent = (cameraSerial: string, filters: CameraSettings) => {
  const event = new CustomEvent(CameraProfileEvents.FILTERS_READY, {
    detail: { cameraSerial, filters },
  });
  window.dispatchEvent(event);
};
