import {configureStore, createListenerMiddleware, isAnyOf} from "@reduxjs/toolkit";
import {rootReducer} from "../RootReducer";
import {FLUSH, PAUSE, PERSIST, persistReducer, persistStore, PURGE, REGISTER, REHYDRATE} from 'redux-persist'
import storage from 'redux-persist/lib/storage';
import {LocalStorageActions} from "../slices/LocalStorageSlice.ts";
import {UIActions} from "../slices/UISlice.ts";
import {tabSyncMiddleware} from "../../utils/crossTabSync.ts"; // defaults to localStorage for web

export default function configureRootStore() {
  const persistConfig = {
    key: 'root',
    storage,
    // only localStorageState and uiState are persisted
    // any stores that want to persist should be added here
    whitelist: ["localStorageState", "uiState"]
  };

  const tabSyncConfig = {
    // whitelist of actions to be synced across tabs upon update
    // any actions that what to be synced across tabs should be added here
    matcher: isAnyOf(
      LocalStorageActions.SET_VALUE,
      UIActions.IP_UPDATE,
    ),
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

