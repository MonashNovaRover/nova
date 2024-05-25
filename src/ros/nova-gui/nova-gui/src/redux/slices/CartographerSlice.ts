import { createSlice, PayloadAction } from "@reduxjs/toolkit";
import {
  CartographerState,
  MapCoordinate,
  MapInteractionMode,
  MapPoint,
} from "../models/CartographerState";

export const cartographerSlice = createSlice({
  reducers: {
    ADD_POINT: (state: CartographerState, action: PayloadAction<MapPoint>) => {
      return {
        ...state,
        points: [...state.points, action.payload],
        newMarkerModal: {
          open: false,
        },
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
    UPDATE_MOUSE_POSITION: (
      state: CartographerState,
      action: PayloadAction<MapCoordinate | undefined>
    ) => {
      return {
        ...state,
        mousePosition: action.payload,
      };
    },
    HANDLE_MAP_CLICK: (
      state: CartographerState,
      action: PayloadAction<MapCoordinate | undefined>
    ) => {
      switch (state.mapInteractionMode) {
        case MapInteractionMode.SELECT: {
          // Add a New Point
          return {
            ...state,
            newMarkerModal: {
              open: true,
              coordinate: action.payload,
            },
          };
        }

        default:
          state;
      }
    },
    CLOSE_ADD_MODAL: (state: CartographerState) => ({
      ...state,
      newMarkerModal: {
        open: false,
      },
    }),
  },
  initialState: <CartographerState>{
    points: [],
    mapInteractionMode: MapInteractionMode.PAN,
    newMarkerModal: {
      open: false,
      coordinate: undefined,
    },
  },
  name: "CartographerReducer",
});

export const CartographerAction = cartographerSlice.actions;
