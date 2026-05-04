// Generates tests/snapshot-big.json with realistic competition-scale state.
// Run: node tests/make-big-snapshot.mjs
import { writeFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";

const __dir = dirname(fileURLToPath(import.meta.url));

const cameraNames = [
  "mast_belly", "mast_forward", "mast_backward", "mast_arm_stow",
  "arm_end_top", "arm_end_finger", "arm_end_periscope", "arm_end_side",
  "arm_wrist", "science_microscope", "science_spectroscope",
  "science_gimbal_cam", "science_cuvettes",
];

// 50 camera profiles, each with all 13 cameras
const profiles = {};
for (let i = 0; i < 50; i++) {
  const cams = {};
  for (const name of cameraNames) {
    cams[name] = {
      flipCamera: i % 2 === 0,
      invertCamera: i % 3 === 0,
      rotation: ((i * 30) % 360) - 180,
      contrast: 50 + ((i * 7) % 150),
      brightness: 50 + ((i * 11) % 150),
    };
  }
  const id = `profile_${i.toString().padStart(3, "0")}`;
  profiles[id] = { name: id, timestamp: 1700000000000 + i * 1000, cameras: cams };
}
const cameraProfiles = { value: { profiles, lastLoadedProfile: null } };

// 4 sites, each with 200 thresholding entries and 100 entries per resource type
const siteValue = {};
for (let s = 0; s < 4; s++) {
  siteValue[s] = {
    siteType: 0,
    spaceResourcesEntries: {
      1: Array.from({ length: 100 }, (_, j) => ({ data: 30000 + j * 10, type: 1, label: `auto_${j}` })),
      2: Array.from({ length: 100 }, (_, j) => ({ data: 30000 + j * 10, type: 2, label: `auto_${j}` })),
    },
    thresholdingEntries: Array.from({ length: 200 }, (_, j) => ({
      timestamp: 1700000000000 + j,
      type: j % 3,
      value: j * 1.5,
      label: `entry_${s}_${j}`,
      coords: { x: j * 0.1, y: j * 0.2, z: j * 0.3 },
    })),
    MLOutput: "",
  };
}
const siteData = { value: siteValue };

// nirProbeCalibrationData with bigger curves
const calibration = {
  value: {
    coefficients: Array.from({ length: 200 }, (_, i) => i * 1e-6),
    xOffset: 0,
    yOffset: 0,
    xRange: [18000, 27000],
    yRange: [18000, 27000],
    history: Array.from({ length: 50 }, (_, i) => ({ ts: i, points: Array.from({ length: 100 }, (_, j) => [j, j * 1.5]) })),
  },
};

// Build the redux-persist blob (each slice's value is a JSON string)
const persisted = {
  uiState: JSON.stringify({
    baseStationIP: "10.0.0.101",
    roverIP: "10.0.0.10",
    settingsModalOpen: false,
    controllerHelpModalOpen: false,
    sidebarIsVisible: false,
    blcmdStatusModalOpen: false,
  }),
  localStorageState: JSON.stringify({
    map: { mapTile: "Hanksville", storedPoints: [], counterObj: { counterOne: 20, counterTwo: 20 } },
  }),
  currentSite: JSON.stringify({ value: 1 }),
  siteData: JSON.stringify(siteData),
  nirProbeCalibrationData: JSON.stringify(calibration),
  counter: JSON.stringify({ value: 20 }),
  scimbalStepSize: JSON.stringify({ value: "1" }),
  targetTemp: JSON.stringify({ value: 150 }),
  waterPumpEffort: JSON.stringify({ value: 0 }),
  diaphragmPumpEffort: JSON.stringify({ value: 0 }),
  theta360CompassHeading: JSON.stringify({ value: 180 }),
  theta360InputDistance: JSON.stringify({ value: "" }),
  rgbLedStore: JSON.stringify({ value: { r: "0", g: "0", b: "0" } }),
  uvVisBlankStore: JSON.stringify({ value: [] }),
  cameraProfiles: JSON.stringify(cameraProfiles),
  clickAndHold: JSON.stringify({ value: false }),
  windowWideWASD: JSON.stringify({ value: false }),
  toolRotatorPresets: JSON.stringify({ value: { sweeper: 0, microscope: 120, nir_probe: 240 } }),
  toolRotatorTwitchStep: JSON.stringify({ value: 5 }),
  toolRotatorKeyboardControl: JSON.stringify({ value: false }),
  pumpDefaultDurations: JSON.stringify({
    value: {
      fill_shots: 10, fill_cuvettes_prime: 10, fill_cuvettes: 10,
      flush_shots: 10, flush_cuvettes: 10, flush_all: 10,
      empty_shots: 10, empty_cuvettes: 10,
    },
  }),
  _persist: JSON.stringify({ version: -1, rehydrated: true }),
};

// Top-level keys (as the new draft/cached hooks would store them — outside of
// the redux-persist blob). Bench needs these so migrated hooks have something
// to read on mount.
const localStorageObj = {
  "persist:nova-gui": JSON.stringify(persisted),
  "counter": JSON.stringify(20),
};

const out = JSON.stringify(localStorageObj);
writeFileSync(join(__dir, "snapshot-big.json"), out);
console.log("wrote snapshot-big.json,", out.length, "bytes");
console.log("persist:nova-gui blob length:", localStorageObj["persist:nova-gui"].length, "bytes");
