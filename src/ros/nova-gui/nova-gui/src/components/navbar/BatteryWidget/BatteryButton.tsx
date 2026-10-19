
import { Button } from "@heroui/react";

import { useEffect } from "react";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";

import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";

import { BatteryModal } from "./BatteryModal.tsx";

import BatteryGauge from 'react-battery-gauge'

export const BatteryWidget = () => {
  
  const uiActions = useUIActions();
  const batteryLevel = useSelector((state: RootState) => state.batteryStore.percentage);
  const bifrost = useBifrost({ topic: RosTopic.BATTERY_STATE });

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  return (
    <div>
      <Button
        radius="sm"
        size="sm"
        onPress={() => uiActions.setBatteryModalOpen(true)}
      >
        <BatteryGauge size={35} value={batteryLevel} customization={{
        batteryBody: { cornerRadius: 10, fill: '#878686', strokeColor: '#fff' },
        batteryCap: { strokeColor: '#fff'},
        batteryMeter: { fill: '#fff', lowBatteryValue: 21, outerGap: 2 },
        readingText: {
          lightContrastColor: '#111', darkContrastColor: '#111', lowBatteryColor: '#111',
          fontSize: 30, y: '53%', showPercentage: false
        }
      }}/>
      </Button>
      <BatteryModal/>
  </div>
  );
};
