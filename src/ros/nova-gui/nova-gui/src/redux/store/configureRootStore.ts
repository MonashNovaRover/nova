import {configureStore, createListenerMiddleware} from "@reduxjs/toolkit";
import {FLUSH, PAUSE, PERSIST, persistReducer, persistStore, PURGE, REGISTER, REHYDRATE} from 'redux-persist'
import storage from 'redux-persist/lib/storage';
import {UIActions} from "../slices/UISlice.ts";
import {TabSyncBlacklist, tabSyncMiddleware, tabSyncPredicate} from "./middleware/crossTabSync.ts";
import {filterStores} from "./rootReducerFilters.ts";
import {rootReducer, reduxStores} from "../RootReducer.ts";
import {BifrostActionTypes} from "../actions/bifrost/createBifrostAction.ts";

export default function configureRootStore() {

  const persistConfig = {
    key: 'nova-gui',
    storage,
    // A blacklist of store names that should not be persisted.
    // Any stores that should not be persisted should be added here
    // unless a StoreContext is used.
    blacklist: [
      ...filterStores(reduxStores, "shouldPersist", false),
      "cartographerState",
      "cameraStreamerState",
      "bifrostStatus",
    ]
  };

  const tabSyncConfig = {
    // Blacklist of actions to not be synced across tabs upon update.
    // Any actions that should not be synced across tabs should be added here
    // unless a StoreContext is used and the first portion of the action is the
    // store name (e.g. generic stores).
    predicate: tabSyncPredicate({
      // Specific action prefixes to not sync (string before the first '/').
      // e.g. The action type "persist/REHYDRATE" will be ignored because "persist"
      // is in the blacklist.
      // This is usually the name given in the "createSlice" function.
      actionPrefixes: [
        ...filterStores(reduxStores, "shouldTabSync", false),
        "CameraStreamReducer",
        "persist",

        // bifrost actions
        ...Object.values(BifrostActionTypes),
      ],
      // Specific action types not to sync (whole action type).
      actions: [
        UIActions.SETTINGS_MODAL_UPDATE.toString(),
        UIActions.CONTROLLER_HELP_MODAL_UPDATE.toString(),
        UIActions.SIDEBAR_UPDATE.toString(),
        UIActions.BLCMD_STATUS_MODAL_UPDATE.toString(),
      ],
    } as TabSyncBlacklist),
    // The function that is called for every action that matches one of the whitelisted actions.
    effect: tabSyncMiddleware(),
  };

  const listenerMiddleware = createListenerMiddleware();

  const store = configureStore({
    reducer: persistReducer(persistConfig, rootReducer),
    devTools: true,

    middleware: (getDefaultMiddleware) =>
      getDefaultMiddleware({
        // complains about non-serialised data in actions if this is not included
        serializableCheck: {
          ignoredActions: [FLUSH, REHYDRATE, PAUSE, PERSIST, PURGE, REGISTER],
        },
      })
        .prepend(listenerMiddleware.middleware),
  });

  // wrap the store in redux-persist
  const persistor = persistStore(store);

  // start tab sync middleware
  listenerMiddleware.startListening(tabSyncConfig);

  return {store, persistor};
}
