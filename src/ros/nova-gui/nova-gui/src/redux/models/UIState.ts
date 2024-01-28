export interface UIState {
  rosUrl: string;
  settingsModalOpen: boolean;
  controllerHelpModalOpen: boolean;
}

export const initialUIState: UIState = {
  rosUrl: "ws://localhost:9090",
  settingsModalOpen: false,
  controllerHelpModalOpen: false,
};
