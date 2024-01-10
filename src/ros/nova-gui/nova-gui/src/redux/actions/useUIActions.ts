import { useDispatch } from "react-redux";
import { UIActions } from "../slices/UISlice";

export function useUIActions() {
  const dispatch = useDispatch();

  return {
    updateROSurl(rosUrl: string) {
      dispatch({ type: UIActions.URL_UPDATE.type, payload: rosUrl });
    },
    setSettingsModal(settingsModalOpen: boolean) {
      dispatch({
        type: UIActions.MODAL_UPDATE.type,
        payload: settingsModalOpen,
      });
    },
  };
}
