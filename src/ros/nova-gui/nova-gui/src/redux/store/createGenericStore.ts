import {createSlice} from "@reduxjs/toolkit";
import {StoreContext, StoreType} from "../models/StoreContext.ts";

/**
 * creates a generic store from the provided name and initial value
 *
 * See the [generic store doc]{@link https://github.com/MonashNovaRover/nova/blob/master/src/ros/nova-gui/docs/generic_store.md}
 * for more information
 *
 * @param name name of the store
 * @param initialValue initial value of the store
 * @param shouldPersist optional, whether the store should persist, defaults to true
 * @param shouldTabSync optional, whether the store should tab sync, defaults to true
 * @returns StoreContext of the generic store
 */
export const createGenericStore = <T>(name: string, initialValue: T, shouldPersist = true, shouldTabSync = true): StoreContext => {

  const reducer = createSlice({
    name: name,
    initialState: {value: initialValue},
    reducers: {
      SET_VALUE: (state, action) => {
        // type check
        if (typeof action.payload != typeof initialValue) {
          console.error(`Type mismatch found for generic store key ${name}, initialValue and stored values types dont ` +
            `match`, initialValue, action.payload);
          throw new Error(`Type mismatch found for generic store key ${name}, initialValue and stored values types dont ` +
          `match`);
        }
        // update value
        if (action.payload instanceof Function) {
          state.value = action.payload(state.value);
        }
        else {
          state.value = action.payload;
        }
      },
    },
  }).reducer;

  return {
    storeType: StoreType.GENERIC,
    initialValue: initialValue,
    reducer: reducer,
    shouldPersist: shouldPersist,
    shouldTabSync: shouldTabSync,
  } as StoreContext
}
