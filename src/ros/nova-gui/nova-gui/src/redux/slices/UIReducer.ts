import { PayloadAction, createSlice } from "@reduxjs/toolkit";
import { UIState, initialUIState } from "../models/UIState";
import { UIActions } from "../actions/useUIActions";

export const uiSlice = createSlice({
  reducers: {
    [UIActions.UPDATE_ROS_URL]: (
      state: UIState,
      action: PayloadAction<string>
    ) => {
      return {
        ...state,
        rosUrl: action.payload,
      };
    },
    [UIActions.SET_SETTINGS_MODAL]: (
      state: UIState,
      action: PayloadAction<boolean>
    ) => {
      return {
        ...state,
        settingsModalOpen: action.payload,
      };
    },
  },
  initialState: initialUIState,
  name: "UIReducer",
});
