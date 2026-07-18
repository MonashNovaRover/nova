import {Reducer} from "@reduxjs/toolkit";

export enum StoreType {
  OTHER,
  BIFROST,
  GENERIC,
}

/**
 * StoreContext gives more context to a designated store
 *
 * Includes the store's initial value and reducer, as well
 * as other config fields.
 */
export interface StoreContext {
  storeType: StoreType;
  initialValue: unknown;
  reducer: Reducer;
  shouldPersist: boolean;
  shouldTabSync: boolean;
}