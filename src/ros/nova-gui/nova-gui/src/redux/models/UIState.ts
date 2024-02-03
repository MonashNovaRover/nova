export interface UIState {
  rosUrl: string;
  settingsModalOpen: boolean;
  controllerHelpModalOpen: boolean;
}

export const initialUIState: UIState = {
  rosUrl: window.localStorage.getItem("baseIP") ?? "192.168.1.81",
  settingsModalOpen: false,
  controllerHelpModalOpen: false,
};
