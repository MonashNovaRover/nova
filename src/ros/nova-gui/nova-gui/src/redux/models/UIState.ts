export interface UIState {
  rosUrl: string;
  settingsModalOpen: boolean;
  controllerHelpModalOpen: boolean;
}

export const initialUIState: UIState = {
  rosUrl: "ws://192.168.64.7:9090",
  settingsModalOpen: false,
  controllerHelpModalOpen: false,
};
