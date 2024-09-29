import {createSlice} from "@reduxjs/toolkit";

export interface GenericStoreState<T> {
  value: T,
}

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
