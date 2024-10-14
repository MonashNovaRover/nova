import {rootReducer, RootReducerKey} from "../RootReducer.ts";
import {combineReducers} from "@reduxjs/toolkit";


/**
 * converts all StoreContexts to reducers from rootReducer
 */
export const getReducers = () => {
  const reducers = Object.keys(rootReducer).reduce((acc, val) => {
    const key = val as RootReducerKey
    if (rootReducer[val as RootReducerKey] instanceof Function)
      return acc

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
      const key = val as RootReducerKey
      if (rootReducer[val as RootReducerKey] instanceof Function)
        return false

      const reducerKey = field as keyof typeof rootReducer[typeof key]
      return rootReducer[key][reducerKey] !== undefined && !rootReducer[key][reducerKey]
    })
}
