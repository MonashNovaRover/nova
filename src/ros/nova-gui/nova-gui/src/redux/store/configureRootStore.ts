import {configureStore, createListenerMiddleware} from "@reduxjs/toolkit";
import {rootReducer} from "../RootReducer";
import {FLUSH, PAUSE, PERSIST, persistReducer, persistStore, PURGE, REGISTER, REHYDRATE} from 'redux-persist'
import storage from 'redux-persist/lib/storage';
import {UIActions} from "../slices/UISlice.ts";
import {tabSyncMiddleware, tabSyncPredicate} from "./middleware/crossTabSync.ts";
import {bifrostStores} from "./bifrost/bifrostStores.ts";

export default function configureRootStore() {
  const persistConfig = {
    key: 'root',
    storage,
    // stores that should not be persisted includes all bifrost stores
    // any stores that should not be persisted should be added here.
    blacklist: [
      ...bifrostStores,
      "cartographerState",
      "cameraStreamerState",
    ]
  };

  const tabSyncConfig = {
    // blacklist of actions to be synced across tabs upon update
    // any actions that should not be synced across tabs should be added here
    predicate: tabSyncPredicate({
      // specific stores not to sync
      stores: [
        "cameraStreamerState",
        "persist",
      ],
      // specific actions not to sync
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

