import { useDispatch } from "react-redux";
import { UIActions } from "../slices/UIReducer";

export function useUIActions() {
  const dispatch = useDispatch();

  return {
    updateROSurl(rosUrl: string) {
      dispatch({ type: UIActions.URL_UPDATE, payload: rosUrl });
    },
    setSettingsModal(settingsModalOpen: boolean) {
      dispatch({
        type: UIActions.SETTINGS_MODAL_UPDATE,
        payload: settingsModalOpen,
      });
    },
    setControllerHelpModal(controllerHelpModalOpen: boolean) {
      dispatch({
        type: UIActions.CONTROLLER_HELP_MODAL_UPDATE,
        payload: controllerHelpModalOpen,
      });
    },
  };
}
