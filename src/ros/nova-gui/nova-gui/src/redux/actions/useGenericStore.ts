import {useDispatch, useSelector} from "react-redux";
import {RootState} from "../RootState.ts";
import {useCallback} from "react";
import {GenericStoreState} from "../models/genericStores/GenericStoreState.ts";

export function useGenericStore<T>(storeName: string) {
  const value = useSelector((store: RootState) => {

    if (!Object.prototype.hasOwnProperty.call(store, storeName))
      throw console.error(`${storeName} is not a store in redux`)

    const valueStore = store[storeName as keyof typeof store] as GenericStoreState<T>;

    if (valueStore.value === undefined)
      throw console.error(`${storeName} is not a generic store`);

    return valueStore.value as T
  })

  const dispatch = useDispatch();
  const setValue = useCallback((value: T) => {
    dispatch({
      type: storeName + "/SET_VALUE",
      payload: value,
    })
  }, [])

  return {value, setValue}
}
