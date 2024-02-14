export interface UIState {
  baseStationIP: string;
  roverIP: string;
  settingsModalOpen: boolean;
  controllerHelpModalOpen: boolean;
}

export const initialUIState: UIState = {
  baseStationIP: window.localStorage.getItem("baseIP") ?? "192.168.1.81",
  roverIP: window.localStorage.getItem("roverIP") ?? "192.168.0.204",
  settingsModalOpen: false,
  controllerHelpModalOpen: false,
};
