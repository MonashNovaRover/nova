import {useDispatch} from "react-redux";
import {useCallback} from "react";
import {reduxStores} from "../redux/RootReducer.ts";
import {StoreType} from "../redux/models/StoreContext.ts";

/**
 * Returns a function that will reset the provided redux store to it's initial value
 *
 * @param reduxStore struct of stores in Reducer or StoreContext forms to reset to.
 * @return whether or not the reset was successful
 */
function useResetGenericStoreWithStore<T, S extends object>(reduxStore: S): (a: string) => boolean {

  // create a setValue function using dispatch
  const dispatch = useDispatch();

  return useCallback((storeName: string)=> {
    const setValue = (value: T) => {
      dispatch({
        type: storeName + "/SET_VALUE",
        payload: value,
      })
    }

    // check there is a store with the provided name in redux
    const key = storeName as keyof typeof reduxStore
    if (reduxStore[key] === undefined) {
      console.error(`${storeName} is not a store in redux`);
      return false;
    }

    // check that the store is a generic store
    const store = reduxStore[key]
    if (store === null) {
      console.error(`${storeName} is not a store`);
      return false
    }

    const typeKey = "storeType" as keyof typeof reduxStore[typeof key]
    if ((store instanceof Function) || store[typeKey] !== StoreType.GENERIC) {
      console.error(`${storeName} is not a generic store`);
      return false
    }

    const initialValueKey = "initialValue" as keyof typeof reduxStore[typeof key]
    setValue(store[initialValueKey] as T);
    return true
  }, [reduxStore, dispatch])
}

/**
 * Returns a function that will reset the provided redux store to it's initial value
 *
 * See the [generic store doc]{@link https://github.com/MonashNovaRover/nova/blob/master/src/ros/nova-gui/docs/generic_store.md}
 * for more information.
 *
 * @return whether or not the reset was successful
 */
export function useResetGenericStore(): (a: string) => boolean {
  return useResetGenericStoreWithStore(reduxStores);
}
