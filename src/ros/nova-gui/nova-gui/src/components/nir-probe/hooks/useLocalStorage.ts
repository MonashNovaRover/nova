import {DependencyList, useCallback, useEffect, useState} from "react";

/**
 * Like useState, but persists using localStorage
 * @param key The local storage key to use
 * @param initialValue The initial value to assign when there is nothing in local storage
 * @param dependencies Dependencies to update the value from local storage when changed
 */
export function useLocalStorage<T>(key: string, initialValue: T, dependencies: DependencyList = [key]) : [T, (newValue: T) => void] {
  const [value, setValue] = useState<T>(initialValue)

  useEffect(() => {
    const storedValueJSON = localStorage.getItem(key);
    if (storedValueJSON === null) {
      setValue(initialValue);
      return;
    }

    // Parse JSON and do some checks to make sure it is the correct type
    const storedValue = JSON.parse(storedValueJSON)
    if (storedValue === undefined) {
      setValue(initialValue);
      return;
    }

    // This value was successfully retrieved from local storage.
    setValue(storedValue);
  }, dependencies)

  const lsSetValue = useCallback((newValue: T) => {
    setValue(newValue);
    localStorage.setItem(key, JSON.stringify(newValue));
  }, [key])

  return [value, lsSetValue];
}