import {createSlice, PayloadAction} from "@reduxjs/toolkit";
import {initialLocalStorageState, LocalStorageState, LSKeyValuePair} from "../models/LocalStorageState.ts";

export const localStorageSlice = createSlice({
    name: "LocalStorageReducer",
    initialState: initialLocalStorageState,
    reducers: {
        SET_VALUE: (state: LocalStorageState, action: PayloadAction<LSKeyValuePair>) => {
            state.map[action.payload.key] = action.payload.value
        },
    },
});

export const LocalStorageActions = localStorageSlice.actions;
