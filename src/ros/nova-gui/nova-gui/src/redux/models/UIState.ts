export interface UIState {
  baseStationIP: string;
  settingsModalOpen: boolean;
  controllerHelpModalOpen: boolean;
}

export const initialUIState: UIState = {
  baseStationIP: window.localStorage.getItem("baseIP") ?? "192.168.1.81",
  settingsModalOpen: false,
  controllerHelpModalOpen: false,
};
