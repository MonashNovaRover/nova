import { useEffect, useRef, useState } from "react";
import { Button, Card, CardBody, CardHeader, Chip } from "@nextui-org/react";
import { useSelector } from "react-redux";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { RootState } from "../../../redux/RootState.ts";
import {
  usePotentiostatStorage,
  calculateMeanOffset,
  applyCalibration,
  setManualOffset,
  type PotentiostatReading,
} from "./potentiostatStorage.ts";
import { PotentiostatChart } from "./PotentiostatChart.tsx";
import { PotentiostatOptionsMenu } from "./PotentiostatOptionsMenu.tsx";
import { CalibrationMenu } from "./CalibrationMenu.tsx";

type WidgetMode = "measurement" | "calibration";

interface PotentiostatWidgetProps {
  isCompact?: boolean;
}

export const PotentiostatWidget = ({ isCompact = false }: PotentiostatWidgetProps) => {
  // Subscribe to potentiostat data topic
  const bifrost = useBifrost({ topic: RosTopic.POTENTIOSTAT_DATA });
  const potentiostatData = useSelector((state: RootState) => state.potentiostatStore);

  // Service to trigger channels
  const triggerBifrost = useBifrost({ service: RosService.POTENTIOSTAT_TRIGGER });

  // Storage hook
  const { data, addReading, clearChannel, clearAll, saveCalibration, clearCalibration } =
    usePotentiostatStorage();

  // Settings state
  const [lockButtonsDuringReading, setLockButtonsDuringReading] = useState(true);
  const [mode, setMode] = useState<WidgetMode>("measurement");
  const [calibratingChannel, setCalibratingChannel] = useState<1 | 2 | null>(null);
  const [calibrationReadings, setCalibrationReadings] = useState<PotentiostatReading[]>([]);

  // Track previous is_receiving state to detect measurement completion
  const wasReceiving = useRef(false);

  // Sync with topic on mount
  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  // Store incoming readings and handle calibration completion
  useEffect(() => {
    if (potentiostatData.is_receiving) {
      const channelNum = (potentiostatData.channel === 0 ? 1 : 2) as 1 | 2;
      const rawReading: PotentiostatReading = {
        voltage: potentiostatData.voltage,
        current: potentiostatData.current,
        time: Date.now(),
      };

      if (mode === "measurement") {
        // Apply calibration offsets before storing
        const offsets = data.calibration[channelNum === 1 ? "channel1" : "channel2"];
        const calibratedReading = applyCalibration(rawReading, offsets);
        addReading(channelNum, calibratedReading);
      } else if (mode === "calibration") {
        // Collect calibration readings (not shown on chart)
        // eslint-disable-next-line react-hooks/set-state-in-effect
        setCalibrationReadings((prev) => [...prev, rawReading]);
      }

      wasReceiving.current = true;
    } else if (wasReceiving.current) {
      // Measurement complete - handle calibration if active
      if (mode === "calibration" && calibratingChannel && calibrationReadings.length > 0) {
        const offsets = calculateMeanOffset(calibrationReadings);
        if (offsets) {
          saveCalibration(calibratingChannel, offsets);
        }
        setCalibrationReadings([]);
        setCalibratingChannel(null);
      }
      wasReceiving.current = false;
    }
  }, [potentiostatData, addReading, mode, calibratingChannel, calibrationReadings, data.calibration, saveCalibration]);

  const triggerChannel = (channel: 0 | 1) => {
    if (mode === "calibration") {
      startCalibration(channel);
    } else {
      triggerBifrost.callService({ option: channel });
    }
  };

  const startCalibration = (channel: 0 | 1) => {
    const channelNum = (channel === 0 ? 1 : 2) as 1 | 2;
    setCalibratingChannel(channelNum);
    setCalibrationReadings([]);
    // Auto-clear channel data when starting calibration
    clearChannel(channelNum);
    triggerBifrost.callService({ option: channel });
  };

  const handleSetManualOffset = (channel: 1 | 2, voltage: number, current: number) => {
    const offsets = setManualOffset(voltage, current);
    saveCalibration(channel, offsets);
  };

  const handleModeChange = (newMode: WidgetMode) => {
    setMode(newMode);
  };

  const buttonsDisabled = lockButtonsDuringReading && potentiostatData.is_receiving;

  return (
    <Card>
      <CardHeader className="flex flex-row justify-between items-center pb-0">
        <span>Potentiostat</span>
        <div className="flex gap-2 items-center">
          <Chip size="sm" variant="flat" color={mode === "calibration" ? "warning" : "default"}>
            {mode === "calibration" ? "Calibration" : "Measurement"}
          </Chip>
          <CalibrationMenu
            mode={mode}
            onSetMode={handleModeChange}
            calibration={data.calibration}
            onSetManualOffset={handleSetManualOffset}
            onClearCalibration={clearCalibration}
          />
          <PotentiostatOptionsMenu
            lockButtonsDuringReading={lockButtonsDuringReading}
            onToggleLock={() => setLockButtonsDuringReading(!lockButtonsDuringReading)}
            channel1Count={data.channel1.length}
            channel2Count={data.channel2.length}
            onClearChannel1={() => clearChannel(1)}
            onClearChannel2={() => clearChannel(2)}
            onClearAll={clearAll}
          />
        </div>
      </CardHeader>
      <CardBody className={`flex flex-col ${isCompact ? "gap-3" : "gap-6"}`}>
        {/* Status and trigger buttons */}
        <div className={`grid grid-cols-8 ${isCompact ? "gap-2" : "gap-3"} items-center place-items-center`}>
          <Chip
            radius="md"
            size={isCompact ? "sm" : "lg"}
            variant="dot"
            color={
              !potentiostatData.is_receiving
                ? "success"
                : potentiostatData.channel === 0
                  ? "primary"
                  : "secondary"
            }
            className={`${isCompact ? "h-7" : "h-10"} border-2 col-span-2 ${
              !potentiostatData.is_receiving
                ? "border-success"
                : potentiostatData.channel === 0
                  ? "border-primary"
                  : "border-secondary"
            }`}
          >
            {potentiostatData.is_receiving
              ? `${potentiostatData.channel === 0 ? "Left" : "Right"} Active`
              : "Idle"}
          </Chip>
          <Button
            className="col-span-3 w-full"
            color="primary"
            size={isCompact ? "sm" : "md"}
            onPress={() => triggerChannel(0)}
            isDisabled={buttonsDisabled}
          >
            {mode === "calibration" ? "Calibrate Left" : "Start Left"}
          </Button>
          <Button
            className="col-span-3 w-full"
            color="secondary"
            size={isCompact ? "sm" : "md"}
            onPress={() => triggerChannel(1)}
            isDisabled={buttonsDisabled}
          >
            {mode === "calibration" ? "Calibrate Right" : "Start Right"}
          </Button>
        </div>

        {/* Scatter plot */}
        <PotentiostatChart
          channel1={data.channel1}
          channel2={data.channel2}
          mode={mode}
          height={isCompact ? 200 : 300}
        />
      </CardBody>
    </Card>
  );
};
