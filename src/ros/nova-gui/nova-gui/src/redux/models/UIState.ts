export interface UIState {
  baseStationIP: string;
  roverIP: string;
  settingsModalOpen: boolean;
  controllerHelpModalOpen: boolean;
  sidebarIsVisible: boolean;
}

export const initialUIState: UIState = {
  baseStationIP: window.localStorage.getItem("baseIP") ?? "10.0.0.101",
  roverIP: window.localStorage.getItem("roverIP") ?? "10.0.0.10",
  settingsModalOpen: false,
  controllerHelpModalOpen: false,
  sidebarIsVisible: false,
};
