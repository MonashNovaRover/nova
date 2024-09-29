import {createSlice} from "@reduxjs/toolkit";

/**
 * creates a generic store reducer from the provided name and initial value
 * @param name name of the store
 * @param initialValue initial value of the store
 */
export const createGenericStore = <T>(name: string, initialValue: T) => {
  return createSlice({
    name: name,
    initialState: {value: initialValue},
    reducers: {
      SET_VALUE: (state, action) => {
        state.value = action.payload;
      },
    },
  }).reducer;
}
