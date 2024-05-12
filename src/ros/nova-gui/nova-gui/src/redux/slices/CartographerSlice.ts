import { createSlice, PayloadAction } from "@reduxjs/toolkit";
import {
  CartographerState,
  MapInteractionMode,
  MapPoint,
} from "../models/CartographerState";

export const cartographerSlice = createSlice({
  reducers: {
    ADD_POINT: (state: CartographerState, action: PayloadAction<MapPoint>) => {
      return {
        ...state,
        points: [...state.points, action.payload],
      };
    },
    TOGGLE_INTERACTION_MODE: (state: CartographerState) => {
      return {
        ...state,
        mapInteractionMode:
          state.mapInteractionMode == MapInteractionMode.PAN
            ? MapInteractionMode.SELECT
            : MapInteractionMode.PAN,
      };
    },
  },
  initialState: <CartographerState>{
    points: [],
    mapInteractionMode: MapInteractionMode.PAN,
  },
  name: "CartographerReducer",
});

export const CartographerAction = cartographerSlice.actions;
