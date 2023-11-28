import { configureStore } from "@reduxjs/toolkit";
import { rootReducer } from "../RootReducer";

export default function configureRootStore() {
  const store = configureStore({
    reducer: rootReducer,
    devTools: true,
  });
  return store;
}
