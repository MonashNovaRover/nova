import {useDispatch, useSelector} from "react-redux";
import {RootState} from "../RootState.ts";
import {useCallback} from "react";
import {GenericStoreState} from "../models/genericStores/GenericStoreState.ts";

/**
 * Like useState except uses a GenericStore in redux
 *
 * See the generic_store.md in the docs folder for more information
 *
 * @param storeName the name of the store in redux
 */
export function useGenericStore<T>(storeName: string): [T, (a: T) => void] {
  // the current value from the store
  const value = useSelector((store: RootState) => {

    // type checks
    if (!Object.prototype.hasOwnProperty.call(store, storeName))
      throw console.error(`${storeName} is not a store in redux`)

    const valueStore = store[storeName as keyof typeof store] as GenericStoreState<T>;

    if (valueStore.value === undefined)
      throw console.error(`${storeName} is not a generic store`);

    return valueStore.value as T
  })

  // create a setValue function using dispatch
  const dispatch = useDispatch();
  const setValue = useCallback((value: T) => {
    dispatch({
      type: storeName + "/SET_VALUE",
      payload: value,
    })
  }, [])

  return [value, setValue]
}
