import { useDispatch } from "react-redux";
import { uiSlice } from "../slices/UIReducer";

export enum UIActions {
  UPDATE_ROS_URL,
  SET_SETTINGS_MODAL,
}

export function useUIActions() {
  const dispatch = useDispatch();

  return {
    updateROSurl(rosUrl: string) {
      dispatch({ type: UIActions.UPDATE_ROS_URL, payload: rosUrl });
    },
    setSettingsModal(settingsModalOpen: boolean) {
      dispatch({
        type: UIActions.SET_SETTINGS_MODAL,
        payload: settingsModalOpen,
      });
    },
  };
}
