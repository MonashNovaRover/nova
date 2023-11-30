export interface UIState {
  rosUrl: string;
  settingsModalOpen: boolean;
}

export const initialUIState: UIState = {
  rosUrl: "ws://192.168.1.5:9090",
  settingsModalOpen: false,
};
