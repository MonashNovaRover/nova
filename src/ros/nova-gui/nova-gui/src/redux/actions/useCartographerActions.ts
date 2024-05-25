import { useDispatch } from "react-redux";
import {
  MapCoordinate,
  MapInteractionMode,
  MapPoint,
} from "../models/CartographerState";
import { CartographerAction } from "../slices/CartographerSlice";

export const useCartographerActions = () => {
  const dispatch = useDispatch();

  const addPoint = (point: MapPoint) =>
    dispatch({
      type: CartographerAction.ADD_POINT.type,
      payload: point,
    });

  const deletePoint = (point: MapPoint) => {
    dispatch({
      type: CartographerAction.REMOVE_POINT,
      payload: point,
    });
  };

  const setInteractionMode = (mode: MapInteractionMode) =>
    dispatch({
      type: CartographerAction.SET_INTERACTION_MODE.type,
      payload: mode,
    });

  const updateMousePosition = (coordinates?: { lat: number; long: number }) =>
    dispatch({
      type: CartographerAction.UPDATE_MOUSE_POSITION.type,
      payload: coordinates,
    });

  const handleMapClickEvent = (coordinates: MapCoordinate) => {
    dispatch({
      type: CartographerAction.HANDLE_MAP_CLICK.type,
      payload: coordinates,
    });
  };

  const closeNewModal = () =>
    dispatch({ type: CartographerAction.CLOSE_ADD_MODAL.type });

  const clearMeasurements = () =>
    dispatch({ type: CartographerAction.CLEAR_MEASURE.type });

  return {
    addPoint,
    deletePoint,
    setInteractionMode,
    updateMousePosition,
    handleMapClickEvent,
    closeNewModal,
    clearMeasurements,
  };
};
