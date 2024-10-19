import {combineReducers} from "@reduxjs/toolkit";

/**
 * converts stores which is a mix of StoreContexts and Reducers
 * into a single reducer
 *
 * @param stores a struct containing stores that are either StoreContexts or Reducers
 */
export const getReducers = <T extends object>(stores: T) => {
  const reducers = Object.keys(stores).reduce((acc, val) => {
    // nothing changes when the store is already a reducer
    const key = val as keyof typeof stores
    if (stores[key] instanceof Function)
      return acc

    // set the store to the reducer when store is a StoreContext
    const reducerKey = "reducer" as keyof typeof stores[typeof key]
    if (stores[key][reducerKey] !== undefined)
      return {...acc, [key]: stores[key][reducerKey]}

    console.log(`root stores item is not a Reducer or a StoreContext: ${stores[key]}`)
    throw Error(`root stores item is not a Reducer or a StoreContext: ${stores[key]}`)
  }, stores)

  return combineReducers(reducers)
}

/**
 * Filters the stores and creates a blacklist which includes all
 * store names that have the designated field marked as false if they
 * are a StoreContext
 *
 * @param stores a struct containing stores that are either StoreContexts or Reducers
 * @param field the field of a StoreContext to create a blacklist from
 */
export const filterRootStores = <T extends object>(stores: T, field: string) => {
  return Object.keys(stores)
    .filter((val) => {
      // ignore non StoreContext types
      const key = val as keyof typeof stores
      if (stores[key] instanceof Function)
        return false

      // check if a StoreContext has the field set to false
      const reducerKey = field as keyof typeof stores[typeof key]
      return stores[key][reducerKey] !== undefined && !stores[key][reducerKey]
    })
}
