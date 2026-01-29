// The max velocity of the wheels, considered 100% on the progress bar, in rad/s
export const DRIVE_VEL_MAX = 1.0;

/**
 * This Index / Map converts BLCMD IDs to wheels on the Rover
 */
export const BLCMD_INDEX: { [id: number]: string } = {
  1: "Front Left Wheel",
  2: "Rear Left Wheel",
  3: "Rear Right Wheel",
  4: "Font Right Wheel",
  5: "Front Left Pivot",
  6: "Rear Left Pivot",
  7: "Rear Right Pivot",
  8: "Front Right Pivot",
};
