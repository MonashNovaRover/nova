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
        type: UIActions.MODAL_UPDATE,
        payload: settingsModalOpen,
      });
    },
  };
}
