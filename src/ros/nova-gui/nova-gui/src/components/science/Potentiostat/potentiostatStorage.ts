import { useCallback, useState, useEffect } from "react";

const POTENTIOSTAT_STORAGE_KEY = "potentiostatData";

export interface PotentiostatReading {
  voltage: number;
  current: number;
  time: number; // timestamp in ms for JSON serialization
}

export interface PotentiostatState {
  channel1: PotentiostatReading[];
  channel2: PotentiostatReading[];
}

const DEFAULT_STATE: PotentiostatState = {
  channel1: [],
  channel2: [],
};

/**
 * Get potentiostat data from localStorage
 */
export function getPotentiostatData(): PotentiostatState {
  try {
    const stored = localStorage.getItem(POTENTIOSTAT_STORAGE_KEY);
    if (!stored) return DEFAULT_STATE;
    return JSON.parse(stored) as PotentiostatState;
  } catch {
    console.error("Failed to parse potentiostat data from localStorage");
    return DEFAULT_STATE;
  }
}

/**
 * Save potentiostat data to localStorage
 */
export function savePotentiostatData(data: PotentiostatState): void {
  try {
    localStorage.setItem(POTENTIOSTAT_STORAGE_KEY, JSON.stringify(data));
  } catch {
    console.error("Failed to save potentiostat data to localStorage");
  }
}

/**
 * Add a reading to the specified channel
 */
export function addReading(channel: 1 | 2, reading: PotentiostatReading): PotentiostatState {
  const data = getPotentiostatData();
  const key = channel === 1 ? "channel1" : "channel2";
  data[key] = [...data[key], reading];
  savePotentiostatData(data);
  return data;
}

/**
 * Clear readings for the specified channel
 */
export function clearChannel(channel: 1 | 2): PotentiostatState {
  const data = getPotentiostatData();
  const key = channel === 1 ? "channel1" : "channel2";
  data[key] = [];
  savePotentiostatData(data);
  return data;
}

/**
 * Clear all potentiostat data
 */
export function clearAllData(): PotentiostatState {
  savePotentiostatData(DEFAULT_STATE);
  return DEFAULT_STATE;
}

/**
 * Hook to access and modify potentiostat data stored in localStorage.
 * Provides reactive state that updates when data changes.
 */
export function usePotentiostatStorage() {
  const [data, setData] = useState<PotentiostatState>(getPotentiostatData);

  // Sync with localStorage on mount and when storage changes
  useEffect(() => {
    const handleStorageChange = (e: StorageEvent) => {
      if (e.key === POTENTIOSTAT_STORAGE_KEY) {
        setData(getPotentiostatData());
      }
    };
    window.addEventListener("storage", handleStorageChange);
    return () => window.removeEventListener("storage", handleStorageChange);
  }, []);

  const addReadingToChannel = useCallback((channel: 1 | 2, reading: PotentiostatReading) => {
    const newData = addReading(channel, reading);
    setData(newData);
  }, []);

  const clearChannelData = useCallback((channel: 1 | 2) => {
    const newData = clearChannel(channel);
    setData(newData);
  }, []);

  const clearAll = useCallback(() => {
    const newData = clearAllData();
    setData(newData);
  }, []);

  return {
    data,
    addReading: addReadingToChannel,
    clearChannel: clearChannelData,
    clearAll,
  };
}
