import { useEffect, useRef, useState } from "react";
import { Button, Card, CardBody, CardHeader, Chip } from "@nextui-org/react";
import { useSelector } from "react-redux";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { RootState } from "../../../redux/RootState.ts";
import { usePotentiostatStorage } from "./potentiostatStorage.ts";
import { PotentiostatChart } from "./PotentiostatChart.tsx";
import { PotentiostatOptionsMenu } from "./PotentiostatOptionsMenu.tsx";

export const PotentiostatWidget = () => {
  // Subscribe to potentiostat data topic
  const bifrost = useBifrost({ topic: RosTopic.POTENTIOSTAT_DATA });
  const potentiostatData = useSelector((state: RootState) => state.potentiostatStore);

  // Service to trigger channels
  const triggerBifrost = useBifrost({ service: RosService.POTENTIOSTAT_TRIGGER });

  // Storage hook
  const { data, addReading, clearChannel, clearAll } = usePotentiostatStorage();

  // Settings state
  const [lockButtonsDuringReading, setLockButtonsDuringReading] = useState(true);

  // Track previous is_receiving state to detect new readings
  const wasReceiving = useRef(false);

  // Sync with topic on mount
  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  // Store incoming readings
  useEffect(() => {
    if (potentiostatData.is_receiving) {
      // Add reading to appropriate channel (channel 0 -> channel1, channel 1 -> channel2)
      const channelNum = (potentiostatData.channel === 0 ? 1 : 2) as 1 | 2;
      addReading(channelNum, {
        voltage: potentiostatData.voltage,
        current: potentiostatData.current,
        time: Date.now(),
      });
      wasReceiving.current = true;
    } else if (wasReceiving.current) {
      // Measurement complete
      wasReceiving.current = false;
    }
  }, [potentiostatData, addReading]);

  const triggerChannel = (channel: 0 | 1) => {
    triggerBifrost.callService({ option: channel });
  };

  const buttonsDisabled = lockButtonsDuringReading && potentiostatData.is_receiving;

  return (
    <Card>
      <CardHeader className="flex flex-row justify-between items-center pb-0">
        <span>Potentiostat</span>
        <PotentiostatOptionsMenu
          lockButtonsDuringReading={lockButtonsDuringReading}
          onToggleLock={() => setLockButtonsDuringReading(!lockButtonsDuringReading)}
          channel1Count={data.channel1.length}
          channel2Count={data.channel2.length}
          onClearChannel1={() => clearChannel(1)}
          onClearChannel2={() => clearChannel(2)}
          onClearAll={clearAll}
        />
      </CardHeader>
      <CardBody className="flex flex-col gap-6">
        {/* Status and trigger buttons */}
        <div className="grid grid-cols-8 gap-3 items-center place-items-center">
          <Chip
            radius="md"
            size="lg"
            variant="dot"
            color={
              !potentiostatData.is_receiving
                ? "success"
                : potentiostatData.channel === 0
                  ? "primary"
                  : "secondary"
            }
            className={`h-10 border-2 col-span-2 ${
              !potentiostatData.is_receiving
                ? "border-success"
                : potentiostatData.channel === 0
                  ? "border-primary"
                  : "border-secondary"
            }`}
          >
            {potentiostatData.is_receiving
              ? `Ch${potentiostatData.channel + 1} Active`
              : "Idle"}
          </Chip>
          <Button
            className="col-span-3 w-full"
            color="primary"
            onPress={() => triggerChannel(0)}
            isDisabled={buttonsDisabled}
          >
            Start Channel 1
          </Button>
          <Button
            className="col-span-3 w-full"
            color="secondary"
            onPress={() => triggerChannel(1)}
            isDisabled={buttonsDisabled}
          >
            Start Channel 2
          </Button>
        </div>

        {/* Scatter plot */}
        <PotentiostatChart
          channel1={data.channel1}
          channel2={data.channel2}
        />
      </CardBody>
    </Card>
  );
};
