import {configureStore, createListenerMiddleware} from "@reduxjs/toolkit";
import {UIActions} from "../slices/UISlice.ts";
import {TabSyncBlacklist, tabSyncMiddleware, tabSyncPredicate} from "./middleware/crossTabSync.ts";
import {filterStores} from "./rootReducerFilters.ts";
import {rootReducer, reduxStores, bifrostStores} from "../RootReducer.ts";
import {BifrostActionTypes} from "../actions/bifrost/createBifrostAction.ts";

export default function configureRootStore() {

  const tabSyncConfig = {
    predicate: tabSyncPredicate({
      actionPrefixes: [
        ...filterStores(reduxStores, "shouldTabSync", false).filter(val => !bifrostStores.includes(val)),
        "CameraStreamReducer",
        "persist",
        ...Object.values(BifrostActionTypes),
      ],
      actions: [
        UIActions.SETTINGS_MODAL_UPDATE.toString(),
        UIActions.CONTROLLER_HELP_MODAL_UPDATE.toString(),
        UIActions.SIDEBAR_UPDATE.toString(),
        UIActions.BLCMD_STATUS_MODAL_UPDATE.toString(),
      ],
    } as TabSyncBlacklist),
    effect: tabSyncMiddleware(),
  };

  const listenerMiddleware = createListenerMiddleware();

  const store = configureStore({
    reducer: rootReducer,
    devTools: true,
    middleware: (getDefaultMiddleware) =>
      getDefaultMiddleware().prepend(listenerMiddleware.middleware),
  });

  listenerMiddleware.startListening(tabSyncConfig);

  return {store};
}
