import { useCallback, useState, useEffect } from "react";

const POTENTIOSTAT_STORAGE_KEY = "potentiostatData";

export interface PotentiostatReading {
  voltage: number;
  current: number;
  time: number; // timestamp in ms for JSON serialization
}

export interface CalibrationOffsets {
  voltageOffset: number; // Volts
  currentOffset: number; // mA
  timestamp: number; // When calibration was set
  isManual: boolean; // True if manually entered, false if calculated
  dataPoints?: number; // Number of readings used (undefined for manual)
}

export interface CalibrationState {
  channel1: CalibrationOffsets | null;
  channel2: CalibrationOffsets | null;
}

export interface PotentiostatState {
  channel1: PotentiostatReading[];
  channel2: PotentiostatReading[];
  calibration: CalibrationState;
}

const DEFAULT_STATE: PotentiostatState = {
  channel1: [],
  channel2: [],
  calibration: {
    channel1: null,
    channel2: null,
  },
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
 * Clear all potentiostat measurement data (preserves calibration)
 */
export function clearAllData(): PotentiostatState {
  const data = getPotentiostatData();
  const clearedData: PotentiostatState = {
    channel1: [],
    channel2: [],
    calibration: data.calibration, // Preserve calibration
  };
  savePotentiostatData(clearedData);
  return clearedData;
}

/**
 * Calculate mean offset from calibration readings
 */
export function calculateMeanOffset(readings: PotentiostatReading[]): CalibrationOffsets | null {
  if (readings.length === 0) {
    return null;
  }

  const sumVoltage = readings.reduce((sum, r) => sum + r.voltage, 0);
  const sumCurrent = readings.reduce((sum, r) => sum + r.current, 0);

  return {
    voltageOffset: sumVoltage / readings.length,
    currentOffset: sumCurrent / readings.length,
    timestamp: Date.now(),
    isManual: false,
    dataPoints: readings.length,
  };
}

/**
 * Create manual offset entry
 */
export function setManualOffset(voltageOffset: number, currentOffset: number): CalibrationOffsets {
  return {
    voltageOffset,
    currentOffset,
    timestamp: Date.now(),
    isManual: true,
  };
}

/**
 * Apply calibration offsets to a reading
 */
export function applyCalibration(
  reading: PotentiostatReading,
  offsets: CalibrationOffsets | null
): PotentiostatReading {
  if (!offsets) return reading;

  return {
    voltage: reading.voltage - offsets.voltageOffset,
    current: reading.current - offsets.currentOffset,
    time: reading.time,
  };
}

/**
 * Save calibration offsets for a channel
 */
export function saveCalibration(channel: 1 | 2, offsets: CalibrationOffsets): PotentiostatState {
  const data = getPotentiostatData();
  const key = channel === 1 ? "channel1" : "channel2";
  data.calibration[key] = offsets;
  savePotentiostatData(data);
  return data;
}

/**
 * Clear calibration for a channel
 */
export function clearCalibration(channel: 1 | 2): PotentiostatState {
  const data = getPotentiostatData();
  const key = channel === 1 ? "channel1" : "channel2";
  data.calibration[key] = null;
  savePotentiostatData(data);
  return data;
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

  const saveCalibrationForChannel = useCallback((channel: 1 | 2, offsets: CalibrationOffsets) => {
    const newData = saveCalibration(channel, offsets);
    setData(newData);
  }, []);

  const clearCalibrationForChannel = useCallback((channel: 1 | 2) => {
    const newData = clearCalibration(channel);
    setData(newData);
  }, []);

  return {
    data,
    addReading: addReadingToChannel,
    clearChannel: clearChannelData,
    clearAll,
    saveCalibration: saveCalibrationForChannel,
    clearCalibration: clearCalibrationForChannel,
  };
}
