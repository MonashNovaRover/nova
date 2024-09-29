import {createSlice} from "@reduxjs/toolkit";

// stores to be added to redux
// must be in the form: <Store Name>: <initial value>

export enum GenericStore {
  CURRENT_SITE
}

export interface GenericStoreState<T> {
  value: T,
}

// export interface GenericStoreAction {
//   store: GenericStore,
//   value: object,
// }

// export const GenericStoreSlice = createSlice({
//   name: "GenericStoreReducer",
//   initialState: initialGenericStore,
//   reducers: {
//     SET_VALUE: (state: GenericStoreState, action: PayloadAction<GenericStoreAction>) => {
//       state.[action.payload.store] = action.payload.store
//     },
//   },
// });

// export const LocalStorageActions = localStorageSlice.actions;


export const createGenericStore = <T>(name: string, initialValue: T) => {

  const slice = createSlice({
    name: name,
    initialState: {value: initialValue},
    reducers: {
      SET_VALUE: (state, action) => {
        state.value = action.payload;
      },
    },
  });

  return slice.reducer;
}
