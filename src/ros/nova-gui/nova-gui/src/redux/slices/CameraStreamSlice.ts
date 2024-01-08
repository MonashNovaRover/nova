import { PayloadAction, createSlice } from "@reduxjs/toolkit";
import {
  Camera,
  CameraStreamerState,
  CameraStreamerStatus,
  initialCameraStreamerState,
} from "../models/CameraStreamState";

export const cameraStreamerSlice = createSlice({
  reducers: {
    UPDATE_STATUS: (
      state: CameraStreamerState,
      action: PayloadAction<CameraStreamerStatus>
    ) => {
      return {
        ...state,
        status: action.payload,
      };
    },
    UPDATE_CAMERAS: (
      state: CameraStreamerState,
      action: PayloadAction<Camera[]>
    ) => {
      return {
        ...state,
        cameras: action.payload,
      };
    },
  },
  initialState: initialCameraStreamerState,
  name: "CameraStreamReducer",
});

export const CameraStreamerAction = cameraStreamerSlice.actions;
