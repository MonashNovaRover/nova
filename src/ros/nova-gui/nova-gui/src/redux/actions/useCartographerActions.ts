import { useDispatch } from "react-redux";
import { MapCoordinate, MapPoint } from "../models/CartographerState";
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

  const toggleMapInteractionMode = () =>
    dispatch({
      type: CartographerAction.TOGGLE_INTERACTION_MODE.type,
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

  return {
    addPoint,
    deletePoint,
    toggleMapInteractionMode,
    updateMousePosition,
    handleMapClickEvent,
    closeNewModal,
  };
};
