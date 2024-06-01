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
    REMOVE_POINT: (
      state: CartographerState,
      action: PayloadAction<MapPoint>
    ) => {
      return {
        ...state,
        points: state.points.filter(
          (point) => point.lat !== action.payload.lat && action.payload.long
        ),
        newMarkerModal: {
          open: false,
        },
      };
    },
    SET_INTERACTION_MODE: (
      state: CartographerState,
      action: PayloadAction<MapInteractionMode>
    ) => {
      return {
        ...state,
        mapInteractionMode: action.payload,
      };
    },
    UPDATE_MOUSE_POSITION: (
      state: CartographerState,
      action: PayloadAction<MapCoordinate | undefined>
    ) => {
      if (state.measure.measuring) {
        return {
          ...state,
          mousePosition: action.payload,
          measure: {
            ...state.measure,
            to: action.payload,
          },
        };
      } else {
        return {
          ...state,
          mousePosition: action.payload,
        };
      }
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

        // Measuring Mode
        case MapInteractionMode.MEASURE: {
          if (state.measure.measuring) {
            return {
              ...state,
              measure: {
                ...state.measure,
                to: action.payload,
                measuring: false,
              },
            };
          } else {
            return {
              ...state,
              measure: {
                from: action.payload,
                measuring: true,
              },
            };
          }
        }

        default:
          state;
      }
    },
    CLEAR_MEASURE: (state: CartographerState) => ({
      ...state,
      measure: {
        measuring: false,
      },
    }),
    CLOSE_ADD_MODAL: (state: CartographerState) => ({
      ...state,
      newMarkerModal: {
        open: false,
      },
    }),
    TOGGLE_ROVER_CENTER: (state: CartographerState) => ({
      ...state,
      centerOnRover: !state.centerOnRover,
    }),
    TOGGLE_TRACK_ROVER: (state: CartographerState) => ({
      ...state,
      trackRover: !state.trackRover,
    }),
    SET_POINTS: (state: CartographerState, action: PayloadAction<MapPoint[]>) => ({
        ...state,
        points: [...action.payload],
    }),
  },
  initialState: <CartographerState>{
    points: [],
    mapInteractionMode: MapInteractionMode.PAN,
    newMarkerModal: {
      open: false,
      coordinate: undefined,
    },
    measure: {
      measuring: false,
    },
    centerOnRover: false,
    trackRover: false,
  },
  name: "CartographerReducer",
});

export const CartographerAction = cartographerSlice.actions;
