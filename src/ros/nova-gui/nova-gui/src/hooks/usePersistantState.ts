// Draft implementation from fix/use-local-storage branch — INTENTIONALLY KEPT
// AS-IS for benchmarking. Has known issues: re-parses per mount (no cache),
// side effects in setState updater, no cross-tab sync, subscriber leak.
import { useState, useEffect } from "react";

const subscribers = new Map<string, Set<(value: unknown) => void>>();

function notifySubscribers<T>(key: string, newValue: T) {
  if (subscribers.has(key)) {
    for (const cb of subscribers.get(key)!) {
      cb(newValue);
    }
  }
}

export function usePersistentState<T>(
  key: string,
  defaultValue: T,
): [T, (value: T | ((prev: T) => T)) => void] {
  const getStoredValue = (): T => {
    const raw = localStorage.getItem(key);
    if (raw === null) return defaultValue;
    try {
      return JSON.parse(raw) as T;
    } catch {
      return defaultValue;
    }
  };

  const [state, setState] = useState<T>(getStoredValue);

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
