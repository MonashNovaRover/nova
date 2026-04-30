export interface ProfileOption {
  displayName: string
  name: string
}

// These should be the ones defined in the cameras3 params
export const defaultCameraProfileOptions: ProfileOption[] = [
  {
    displayName: "Default",
    name: "default",
  },
  {
    displayName: "Super",
    name: "super",
  },
  {
    displayName: "Still",
    name: "still",
  },
  {
    displayName: "Snail",
    name: "snail",
  },
  {
    displayName: "Emergency",
    name: "emergency",
  },
]

// These should be the ones defined in the cameras3 params
export const defaultCameraProfilePresets: ProfileOption[] = [
  {
    displayName: "Drive",
    name: "drive",
  },
  {
    displayName: "Payload",
    name: "payload",
  },
  {
    displayName: "Default",
    name: "default",
  },
  {
    displayName: "Super",
    name: "super",
  },
  {
    displayName: "Still",
    name: "still",
  },
  {
    displayName: "Snail",
    name: "snail",
  },
  {
    displayName: "Emergency",
    name: "emergency",
  },
]
