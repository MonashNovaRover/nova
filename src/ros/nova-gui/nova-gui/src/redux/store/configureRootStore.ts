import { configureStore } from "@reduxjs/toolkit";
import { rootReducer } from "../slices/RootReducer";

export default function configureRootStore() {
  const store = configureStore({
    reducer: rootReducer,
    devTools: true,
  });
  return store;
}
