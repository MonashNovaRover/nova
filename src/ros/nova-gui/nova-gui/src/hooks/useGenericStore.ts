import {useDispatch, useSelector} from "react-redux";
import {RootState} from "../redux/RootState.ts";
import {useCallback} from "react";
import {GenericStoreState} from "../redux/models/genericStores/GenericStoreState.ts";

/**
 * Returns a stateful value stored in redux, and a function to update it.
 *
 * See the [generic store doc]{@link https://github.com/MonashNovaRover/nova/blob/master/src/ros/nova-gui/docs/generic_store.md}
 * for more information
 *
 * @param storeName the name of the store in redux
 */
export function useGenericStore<T>(storeName: string): [T, (a: T) => void] {
  // the current value from the store
  const value = useSelector((store: RootState) => {

    // type checks
    if (!Object.prototype.hasOwnProperty.call(store, storeName)) {
      console.error(`${storeName} is not a store in redux`);
      throw new Error(`${storeName} is not a store in redux`);
    }

    const valueStore = store[storeName as keyof typeof store] as GenericStoreState<T>;

    if (valueStore.value === undefined) {
      console.error(`${storeName} is not a generic store`);
      throw new Error(`${storeName} is not a generic store`);
    }

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
