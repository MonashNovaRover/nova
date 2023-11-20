import { createBifrostStore } from "../store/createBifrostStore";
import { uiSlice } from "./UIReducer";

export const rootReducer = {
  uiState: uiSlice.reducer,
  radioState: createBifrostStore<IRadioStatus>({ ping: 0 }),
};
