import {configureStore, createListenerMiddleware} from "@reduxjs/toolkit";
import {rootReducer} from "../RootReducer";
import {FLUSH, PAUSE, PERSIST, persistReducer, persistStore, PURGE, REGISTER, REHYDRATE} from 'redux-persist'
import storage from 'redux-persist/lib/storage';
import {UIActions} from "../slices/UISlice.ts";
import {tabSyncMiddleware, tabSyncPredicate} from "./middleware/crossTabSync.ts";

export default function configureRootStore() {
  const persistConfig = {
    key: 'root',
    storage,
    // only localStorageState and uiState are persisted
    // any stores that want to persist should be added here
    whitelist: ["localStorageState", "uiState", "counter"]
  };

  const tabSyncConfig = {
    // whitelist of actions to be synced across tabs upon update
    // any actions that what to be synced across tabs should be added here
    predicate: tabSyncPredicate({
      stores: [
        "cameraStreamerState",
        "persist",
      ],
      actions: [
        UIActions.SETTINGS_MODAL_UPDATE.toString(),
        UIActions.CONTROLLER_HELP_MODAL_UPDATE.toString(),
        UIActions.SIDEBAR_UPDATE.toString(),
        UIActions.BLCMD_STATUS_MODAL_UPDATE.toString(),
      ],
    }),
    // the function that is called for every action that matches one of the whitelisted actions.
    effect: tabSyncMiddleware(),
  };

  const listenerMiddleware = createListenerMiddleware();

  const store = configureStore({
    reducer: persistReducer(persistConfig, rootReducer),
    devTools: true,

    // complains about non-serialised data in actions if this is not included
    middleware: (getDefaultMiddleware) =>
      getDefaultMiddleware({
        serializableCheck: {
          ignoredActions: [FLUSH, REHYDRATE, PAUSE, PERSIST, PURGE, REGISTER],
        },
      })
        .prepend(listenerMiddleware.middleware),
  });

  // wrap the store in redux-persist
  const persistor = persistStore(store);

  listenerMiddleware.startListening(tabSyncConfig);

  return {store, persistor};
}

