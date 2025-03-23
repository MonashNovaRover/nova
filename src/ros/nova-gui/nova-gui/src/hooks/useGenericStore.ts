import {useDispatch, useSelector} from "react-redux";
import {RootState} from "../redux/RootState.ts";
import {useCallback, useMemo} from "react";
import {GenericStoreState} from "../redux/models/genericStores/GenericStoreState.ts";
import {reduxStores} from "../redux/RootReducer.ts";
import {StoreType} from "../redux/models/StoreContext.ts";

/**
 * Returns a stateful value stored in redux, and a function to update it.
 *
 * @param reduxStore struct of stores in Reducer or StoreContext forms to check types against.
 * @param storeName name of the store in redux to query and update.
 */
function useGenericStoreWithStore<T, S extends object>(reduxStore: S, storeName: string): [T, (a: T | ((b: T) => T)) => void] {
  // the initial value of this store to use for type checks
  const initialValue = useMemo(() => {
    // check there is a store with the provided name in redux
    const key = storeName as keyof typeof reduxStore
    if (reduxStore[key] === undefined) {
      console.error(`${storeName} is not a store in redux`);
      throw new Error(`${storeName} is not a store in redux`);
    }

    // check that the store is a generic store
    const store = reduxStore[key]
    if (store === null) {
      throw new Error(`${storeName} is not a store`)
    }

    const typeKey = "storeType" as keyof typeof reduxStore[typeof key]
    if ((store instanceof Function) || store[typeKey] !== StoreType.GENERIC) {
      console.error(`${storeName} is not a generic store`);
      throw new Error(`${storeName} is not a generic store`);
    }

    const initialValueKey = "initialValue" as keyof typeof reduxStore[typeof key]
    return store[initialValueKey] as T;
  }, [storeName, reduxStore]);

  // create a setValue function using dispatch
  const dispatch = useDispatch();
  const setValue = useCallback((value: T | ((v: T) => T)) => {
    dispatch({
      type: storeName + "/SET_VALUE",
      payload: value,
    })
  }, [storeName, dispatch])

  // the current value from the store
  const value = useSelector((wholeStore: RootState) => {
    const genericStore = wholeStore[storeName as keyof typeof wholeStore] as GenericStoreState<T>;
    const storeValue = genericStore.value

    // if the value in local storage does not match the initial value, set to initial value
    if (typeof storeValue !== typeof initialValue)
      setValue(initialValue)

    return storeValue as T
  })

  return [value, setValue]
}

/**
 * Returns a stateful value stored in redux, and a function to update it.
 *
 * See the [generic store doc]{@link https://github.com/MonashNovaRover/nova/blob/master/src/ros/nova-gui/docs/generic_store.md}
 * for more information.
 *
 * @param storeName the name of the store in redux
 */
export function useGenericStore<T>(storeName: string): [T, (a: T) => void] {
  return useGenericStoreWithStore(reduxStores, storeName);
}
