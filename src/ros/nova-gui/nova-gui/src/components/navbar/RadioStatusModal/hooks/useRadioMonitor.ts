import { useEffect } from "react";

import { RosTopic } from "../../../../ros/topics/rosTopic";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction";


export function useRadioMonitor() {

  const bifrost = useBifrost({ topic: RosTopic.RADIO_STATUS });
  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  return 'Strong';
}
