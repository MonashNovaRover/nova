export interface UIState {
  baseStationIP: string;
  roverIP: string;
  settingsModalOpen: boolean;
  controllerHelpModalOpen: boolean;
  sidebarIsVisible: boolean;
  blcmdStatusModalOpen: boolean;
  radioStatusModalOpen: boolean;
}

export const initialUIState: UIState = {
  baseStationIP: "10.0.0.101",
  roverIP: "10.0.0.10",
  settingsModalOpen: false,
  controllerHelpModalOpen: false,
  sidebarIsVisible: false,
  blcmdStatusModalOpen: false,
  radioStatusModalOpen: false,
};
