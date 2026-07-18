import { PayloadAction, createSlice } from "@reduxjs/toolkit";
import { UIState, initialUIState } from "../models/UIState";

export const uiSlice = createSlice({
  reducers: {
    IP_UPDATE: (
      state: UIState,
      action: PayloadAction<{ roverIP: string; baseStationIP: string }>
    ) => {
      return {
        ...state,
        ...action.payload,
      };
    },

    SETTINGS_MODAL_UPDATE: (state: UIState, action: PayloadAction<boolean>) => {
      return {
        ...state,
        settingsModalOpen: action.payload,
      };
    },
    CONTROLLER_HELP_MODAL_UPDATE: (
      state: UIState,
      action: PayloadAction<boolean>
    ) => {
      return {
        ...state,
        controllerHelpModalOpen: action.payload,
      };
    },

    SIDEBAR_UPDATE: (
      state: UIState,
      action: PayloadAction<boolean>
    ) => {
      return {
        ...state,
        sidebarIsVisible: action.payload,
      };
    },
    BLCMD_STATUS_MODAL_UPDATE: (
      state: UIState,
      action: PayloadAction<boolean>
    ) => {
      return {
        ...state,
        blcmdStatusModalOpen: action.payload,
      };
    },
    RADIO_STATUS_MODAL_UPDATE: (
      state: UIState,
      action: PayloadAction<boolean>
    ) => {
      return {
        ...state,
        radioStatusModalOpen: action.payload,
      };
    },
  },
  initialState: initialUIState,
  name: "UIReducer",
});

export const UIActions = uiSlice.actions;
