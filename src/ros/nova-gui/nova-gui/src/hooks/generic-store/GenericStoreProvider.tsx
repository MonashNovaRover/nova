import React, {createContext, useReducer} from "react";
import {defaultGenericStore} from "./defaultGenericStore.tsx";
import {RootGenericStoreState} from "./rootGenericStoreState.tsx";

type GenericStoreReducer = (arg0: RootGenericStoreState) => RootGenericStoreState

export const GenericStoreProvider = ({children}: {children: React.ReactNode}) => {

  const [store, dispatch] = useReducer(genericStoreReducer, defaultGenericStore)

  return (
    <GenericStoreContext.Provider value={store}>
      <GenericStoreDispatchContext.Provider value={dispatch}>
        {children}
      </GenericStoreDispatchContext.Provider>
    </GenericStoreContext.Provider>
  )


}

const genericStoreReducer: GenericStoreReducer = (store: RootGenericStoreState): RootGenericStoreState => {

  return store



}

export const GenericStoreContext = createContext<RootGenericStoreState>(defaultGenericStore);
export const GenericStoreDispatchContext = createContext<GenericStoreReducer>(genericStoreReducer);
