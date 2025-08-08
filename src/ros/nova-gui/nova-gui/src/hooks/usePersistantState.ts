import { useState, useEffect } from "react";

// Subscribers map: key → set of subscriber callbacks
const subscribers = new Map<string, Set<(value: unknown) => void>>();

/**
 * Notify all subscribers for a given key
 */
function notifySubscribers<T>(key: string, newValue: T) {
  if (subscribers.has(key)) {
    for (const cb of subscribers.get(key)!) {
      cb(newValue);
    }
  }
}

/**
 * Persistent state hook that works like useState,
 * but stores the value in localStorage and syncs between components.
 *
 * @param key - localStorage key
 * @param defaultValue - default value if nothing is stored yet
 * @param debug - whether to apply additional type checks
 * @returns [state, setState] tuple
 */
export function usePersistentState<T>(
  key: string,
  defaultValue: T,
  debug: boolean=false,
): [T, (value: T | ((prev: T) => T)) => void] {
  const getStoredValue = (): T => {
    const raw = localStorage.getItem(key);
    if (raw === null) return defaultValue;
    try {
      return JSON.parse(raw) as T;
    } catch {
      // If it’s not valid JSON, fall back
      return defaultValue;
    }
  };

  // State initialization
  const [state, setState] = useState<T>(getStoredValue);

  // Subscribe to updates for this key
  useEffect(() => {
    const callback = (val: unknown) => setState(val as T);

    if (!subscribers.has(key)) {
      subscribers.set(key, new Set());
    }
    subscribers.get(key)!.add(callback);

    return () => {
      subscribers.get(key)!.delete(callback);
    };
  }, [key]);

  /**
   * Update state and sync to localStorage + all subscribers
   */
  const updateState = (valueOrUpdater: T | ((prev: T) => T)) => {
    setState((prev) => {
      const newValue =
        typeof valueOrUpdater === "function"
          ? (valueOrUpdater as (prev: T) => T)(prev)
          : valueOrUpdater;

      localStorage.setItem(key, JSON.stringify(newValue));
      notifySubscribers(key, newValue);

      return newValue;
    });
  };

  return [state, updateState];
}