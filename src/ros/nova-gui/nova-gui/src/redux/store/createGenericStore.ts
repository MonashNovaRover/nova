import {createSlice} from "@reduxjs/toolkit";

/**
 * creates a generic store reducer from the provided name and initial value
 *
 * see docs/generic_store.md for more information for how to use
 *
 * @param name name of the store
 * @param initialValue initial value of the store
 */
export const createGenericStore = <T>(name: string, initialValue: T) => {

  return createSlice({
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
        state.value = action.payload;
      },
    },
  }).reducer;
}
