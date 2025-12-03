import { useEffect } from "react";
import { useSelector } from "react-redux";

import { RootState } from "../../../../redux/RootState.ts";
import { RadioConnectionStatus } from "../RadioTypes.ts";
import { RosTopic } from "../../../../ros/topics/rosTopic";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction";

// Thresholds
const SIGNAL_LOST = -96;
const SIGNAL_WEAK = -85;
const PING_FAILED = 9999;

export function useRadioMonitor(): RadioConnectionStatus {

  const bifrost = useBifrost({ topic: RosTopic.RADIO_STATUS });
  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const radioStatus = useSelector((state: RootState) => state.radioStore);

  // Check if we've received any radio data yet
  if (!radioStatus.stamp) {
    return RadioConnectionStatus.STARTING;
  }

  // Check signal and ping for lost connection
  if (radioStatus.signal <= SIGNAL_LOST || radioStatus.ping >= PING_FAILED) {
    return RadioConnectionStatus.LOST;
  }

  // Check signal for weak connection
  if (radioStatus.signal <= SIGNAL_WEAK) {
    return RadioConnectionStatus.WEAK;
  }

  return RadioConnectionStatus.STRONG;
}
