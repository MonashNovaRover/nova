import { useEffect, useState } from "react";
import { useSelector } from "react-redux";

import { RootState } from "../../../../redux/RootState.ts";
import { RadioConnectionStatus } from "../RadioTypes.ts";
import { RosTopic } from "../../../../ros/topics/rosTopic";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction";

// Thresholds
const SIGNAL_LOST = -96;
const SIGNAL_WEAK = -85;
const PING_FAILED = 9999;
const MONITOR_TIMEOUT = 3000;

export function useRadioMonitor(): RadioConnectionStatus {

  const bifrost = useBifrost({ topic: RosTopic.RADIO_STATUS });
  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const radioStatus = useSelector((state: RootState) => state.radioStore);

  const [health, setHealth] = useState(RadioConnectionStatus.STARTING);
  const [updateTime, setUpdateTime] = useState<number | null>(null);

  useEffect(() => {
    // Check if we've received any data yet
    if (!radioStatus.stamp) {
      setHealth(RadioConnectionStatus.STARTING);
      return;
    } else {
      setUpdateTime(Date.now());
    }

    // Lost
    if (radioStatus.signal <= SIGNAL_LOST || radioStatus.ping >= PING_FAILED) {
      setHealth(RadioConnectionStatus.LOST);
      return;
    }

    // Weak
    if (radioStatus.signal <= SIGNAL_WEAK) {
      setHealth(RadioConnectionStatus.WEAK);
      return;
    }

    // Strong
    setHealth(RadioConnectionStatus.STRONG);

  }, [radioStatus]);

  // Check whether monitor is still publishing data 
  useEffect(() => {
    const id = setTimeout(() => {
      if (updateTime && Date.now() - updateTime > MONITOR_TIMEOUT) {
        setHealth(RadioConnectionStatus.ERROR);
      }
    }, MONITOR_TIMEOUT);

    return () => clearTimeout(id);
  }, [updateTime]);

  return health;
}
