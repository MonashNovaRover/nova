import {Reducer} from "@reduxjs/toolkit";

/**
 * StoreContext gives more context to a designated store
 *
 * Includes the store's initial value and reducer, as well
 * as other config fields.
 */
export interface StoreContext {
  name: string;
  initialValue: unknown;
  reducer: Reducer;
  shouldPersist: boolean;
  shouldTabSync: boolean;
}