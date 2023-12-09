import { PayloadAction, createSlice } from "@reduxjs/toolkit";
import { UIState, initialUIState } from "../models/UIState";

export const uiSlice = createSlice({
  reducers: {
    URL_UPDATE: (state: UIState, action: PayloadAction<string>) => {
      return {
        ...state,
        rosUrl: action.payload,
      };
    },
    MODAL_UPDATE: (state: UIState, action: PayloadAction<boolean>) => {
      return {
        ...state,
        settingsModalOpen: action.payload,
      };
    },
  },
  initialState: initialUIState,
  name: "UIReducer",
});

export const UIActions = uiSlice.actions;
