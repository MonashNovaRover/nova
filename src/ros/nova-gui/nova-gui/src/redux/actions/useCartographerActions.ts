import { useDispatch } from "react-redux";
import { MapPoint } from "../models/CartographerState";
import { CartographerAction } from "../slices/CartographerSlice";

export const useCartographerActions = () => {
  const dispatch = useDispatch();

  const addPoint = (point: MapPoint) =>
    dispatch({
      type: CartographerAction.ADD_POINT.type,
      payload: point,
    });

  const toggleMapInteractionMode = () =>
    dispatch({
      type: CartographerAction.TOGGLE_INTERACTION_MODE,
    });

  return { addPoint, toggleMapInteractionMode };
};
