import {rootReducer, RootReducerKey} from "../RootReducer.ts";
import {combineReducers} from "@reduxjs/toolkit";

/**
 * converts rootReducer which is a mix of StoreContexts and Reducers
 * into a single reducer
 */
export const getReducers = () => {
  const reducers = Object.keys(rootReducer).reduce((acc, val) => {
    // nothing changes when the store is already a reducer
    const key = val as RootReducerKey
    if (rootReducer[val as RootReducerKey] instanceof Function)
      return acc

    // set the store to the reducer when store is a StoreContext
    const reducerKey = "reducer" as keyof typeof rootReducer[typeof key]
    if (rootReducer[key][reducerKey] !== undefined)
      return {...acc, [key]: rootReducer[key][reducerKey]}

    console.log(`root reducer item is not a Reducer or a StoreContext: ${rootReducer[key]}`)
    throw Error(`root reducer item is not a Reducer or a StoreContext: ${rootReducer[key]}`)
  }, rootReducer)

  return combineReducers(reducers)
}

/**
 * Filters the rootReducer and creates a blacklist which includes all
 * store names that have the designated field marked as false if they
 * are a StoreContext
 *
 * @param field the field of a StoreContext to create a blacklist from
 */
export const filterRootReducer = (field: string) => {
  return Object.keys(rootReducer)
    .filter((val) => {
      // ignore non StoreContext types
      const key = val as RootReducerKey
      if (rootReducer[val as RootReducerKey] instanceof Function)
        return false

      // check if a StoreContext has the field set to false
      const reducerKey = field as keyof typeof rootReducer[typeof key]
      return rootReducer[key][reducerKey] !== undefined && !rootReducer[key][reducerKey]
    })
}
