import {DependencyList, useCallback, useEffect} from "react";
import {useSelector} from "react-redux";
import {RootState} from "../redux/RootState.ts";
import {useLocalStorageActions} from "../redux/actions/useLocalStorageActions.ts";
import {isArray, isObject} from "lodash";

export type NoUnion<T, U = T> = T extends U ? [U] extends [T] ? T : never : never;
export type NoConflict<T> = T & Exclude<T, undefined> & NoUnion<T>

/**
 * Like useState, but persists using localStorage and redux
 * Note: T cannot be a union type, or undefined
 *
 * @deprecated please use useGenericStore instead
 *
 * @param key The local storage key to use
 * @param initialValue The initial value to assign when there is nothing in local storage
 * @param dependencies Dependencies to update the value from local storage when changed
 */
export function useLocalStorage<T>(key: string, initialValue: NoConflict<T>, dependencies: DependencyList = [key])
  : [NoConflict<T>, (newValue: NoConflict<T> | ((previousValue: NoConflict<T>) => NoConflict<T>)) => void] {

  // redux version of value and setValue
  const setValue = useLocalStorageActions().setValue
  const rawValue = useSelector((state: RootState) => state.localStorageState.map[key]) as T;

  // rawValue will be undefined on first render
  const value: NoConflict<T> = rawValue === undefined ? initialValue : (rawValue as NoConflict<T>);

  const onTypeMismatch = useCallback((actualValue: unknown, message: string) => {
    console.error(`Type mismatch found for localStorage key ${key}, initialValue and stored values types dont ` +
      `match.\n\t${message}`, initialValue, actualValue);

    throw new Error(`Type mismatch found for localStorage key ${key}, initialValue and stored values types dont ` +
      `match.\n\t${message}`);
  }, [key, initialValue]);

  // parse local storage and set value to whatever is stored there
  useEffect(() => {
    // Ensure basic types are the same
    if (typeof value !== typeof initialValue)
      onTypeMismatch(value, `Basic type mismatch. ${typeof value} !== ${typeof initialValue}`);

    // Arrays show up as objects so we need another check
    else if (isArray(value) !== isArray(initialValue))
      onTypeMismatch(value, `One value is an array while the other is not...`);

    // Ensure objects have the same keys
    else if (isObject(value) && isObject(initialValue)) {
      const storedValueKeys = Object.keys(value);
      const objectsMatch = Object.keys(initialValue).every(v => storedValueKeys.includes(v));

      if (!objectsMatch)
        onTypeMismatch(value, `Object keys do not match.`);
    }

    // The value is a valid value and matches the type of what is in storage.
    setValue({key: key, value: value});
  }, dependencies)

  const lsSetValue = useCallback((newValue: NoConflict<T> | ((previousValue: NoConflict<T>) => NoConflict<T>)) => {
    const evaluatedNewValue = newValue instanceof Function ? newValue(value) : newValue

    setValue({key: key, value: evaluatedNewValue});
  }, [key, value, setValue]);

  return [value, lsSetValue];
}

