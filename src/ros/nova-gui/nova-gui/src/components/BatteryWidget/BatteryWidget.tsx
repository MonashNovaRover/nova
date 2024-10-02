import { useEffect } from "react";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { RosTopic } from "../../ros/topics/rosTopic";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";

export const BatteryWidget = ({
  size = "28px",
  batteryOutlineColor = "#B0AFAF",
  lowBatteryColor = "#f00",
  fullBatteryColor = "#FFF",
  colorChangeThreshold = 20,
}) => {
  
  const batteryLevel = useSelector((state: RootState) => state.batteryStore.voltage);
  const clampedLevel = Math.max(0, Math.min(batteryLevel, 100));
  const batteryFillColor = clampedLevel > colorChangeThreshold ? fullBatteryColor : lowBatteryColor;
  const bifrost = useBifrost({ topic: RosTopic.BATTERY_STATE });
  
  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  return (
    <svg
      xmlns="http://www.w3.org/2000/svg"
      viewBox="0 0 24 24"
      width={size} height={size}
    >
      <rect 
        x="2" y="6"  width="19" height="12" rx="2" ry="2"
        stroke={batteryOutlineColor} fill="none" strokeWidth="1"
      />
      <rect 
        x="21" y="9.5" width="2" height="5" 
        stroke={batteryOutlineColor} fill="none" strokeWidth="1"
      />
      <rect 
        x="3" y="7" rx="1" ry="1" width={`${clampedLevel * 0.17}px`}  height="10"
        fill={batteryFillColor}
      />
    </svg>
  );
};

