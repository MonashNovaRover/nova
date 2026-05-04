import { useCallback, useSyncExternalStore } from "react";

const cache = new Map<string, unknown>();
const subscribers = new Map<string, Set<() => void>>();

function safeParse<T>(raw: string, fallback: T): T {
  try {
    return JSON.parse(raw) as T;
  } catch {
    return fallback;
  }
}

function readFromStorage<T>(key: string, initial: T): T {
  if (cache.has(key)) return cache.get(key) as T;
  const raw = localStorage.getItem(key);
  const value = raw === null ? initial : safeParse(raw, initial);
  cache.set(key, value);
  return value;
}

function writeToStorage<T>(key: string, value: T) {
  cache.set(key, value);
  localStorage.setItem(key, JSON.stringify(value));
  subscribers.get(key)?.forEach((cb) => cb());
}

if (typeof window !== "undefined") {
  window.addEventListener("storage", (e) => {
    if (e.key === null) return;
    cache.delete(e.key);
    subscribers.get(e.key)?.forEach((cb) => cb());
  });
}

export function usePersistentState<T>(
  key: string,
  initial: T,
): [T, (value: T | ((prev: T) => T)) => void] {
  const subscribe = useCallback(
    (cb: () => void) => {
      let set = subscribers.get(key);
      if (!set) {
        set = new Set();
        subscribers.set(key, set);
      }
      set.add(cb);
      return () => {
        set!.delete(cb);
        if (set!.size === 0) subscribers.delete(key);
      };
    },
    [key],
  );

  const getSnapshot = useCallback(
    () => readFromStorage(key, initial),
    [key, initial],
  );

  const value = useSyncExternalStore(subscribe, getSnapshot);

  const setValue = useCallback(
    (next: T | ((prev: T) => T)) => {
      const resolved =
        typeof next === "function"
          ? (next as (prev: T) => T)(readFromStorage(key, initial))
          : next;
      writeToStorage(key, resolved);
    },
    [key, initial],
  );

  return [value, setValue];
}
