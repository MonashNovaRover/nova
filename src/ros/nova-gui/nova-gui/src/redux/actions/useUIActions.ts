import { useDispatch } from "react-redux";
import { UIActions } from "../slices/UISlice";

export function useUIActions() {
  const dispatch = useDispatch();

  return {
    updateIP(baseStationIP: string, roverIP: string) {
      dispatch({
        type: UIActions.IP_UPDATE.toString(),
        payload: {
          baseStationIP,
          roverIP,
        },
      });
    },
    setSettingsModal(settingsModalOpen: boolean) {
      dispatch({
        type: UIActions.SETTINGS_MODAL_UPDATE.toString(),
        payload: settingsModalOpen,
      });
    },
    setControllerHelpModal(controllerHelpModalOpen: boolean) {
      dispatch({
        type: UIActions.CONTROLLER_HELP_MODAL_UPDATE.toString(),
        payload: controllerHelpModalOpen,
      });
    },
  };
}
