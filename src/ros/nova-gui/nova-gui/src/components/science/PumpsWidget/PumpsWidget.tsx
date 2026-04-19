import {
  Button, Card, CardBody, CardHeader, CardProps, Input, Progress, Select, SelectItem,
  SharedSelection, useDisclosure
} from "@nextui-org/react";
import React, {useCallback, useEffect, useState} from "react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import {Database, MoreHorizontal, Square, Zap} from "react-feather";
import { useGenericStore } from "../../../hooks/useGenericStore.ts";
import PumpsModal from "./PumpsModal.tsx";
import {Radioactive, RecordCircle, RecordCircleFill} from "react-bootstrap-icons";

export interface PumpsWidgetProps extends CardProps {}

export interface PumpData {
  display: string
  value: string
  leftIcon: React.JSX.Element;
  rightIcon: React.JSX.Element;
}

export const PUMPS: PumpData[] = [
  {
    display: "→ Shots",
    value: "cache_to_shot_pump",
    leftIcon: <Square className="w-20"/>,
    rightIcon: <Database className="w-20"/>,
  },
  {
    display: "→ Inner Ring (P)",
    value: "shot_to_inner_pump/prime",
    leftIcon: <Database className="w-20"/>,
    rightIcon: <RecordCircle className="w-20" size={24}/>,
  },
  {
    display: "→ Inner Ring",
    value: "shot_to_inner_pump",
    leftIcon: <Database className="w-20"/>,
    rightIcon: <RecordCircle className="w-20" size={24}/>,
  },
  {
    display: "→ Outer Ring (P)",
    value: "shot_to_outer_pump/prime",
    leftIcon: <Database className="w-20"/>,
    rightIcon: <RecordCircleFill className="w-20" size={24}/>,
  },
  {
    display: "→ Outer Ring",
    value: "shot_to_outer_pump",
    leftIcon: <Database className="w-20"/>,
    rightIcon: <RecordCircleFill className="w-20" size={24}/>,
  },
  {
    display: "→ Electrochem (P)",
    value: "shot_to_electrochem_pump/prime",
    leftIcon: <Database className="w-20"/>,
    rightIcon: <Zap className="w-20"/>,
  },
  {
    display: "→ Electrochem",
    value: "shot_to_electrochem_pump",
    leftIcon: <Database className="w-20"/>,
    rightIcon: <Zap/>,
  },
  {
    display: "→ Sulphuric Acid",
    value: "fill_sulphuric_acid",
    leftIcon: <Radioactive className="w-20" size={24}/>,
    rightIcon: <RecordCircleFill className="w-20" size={24}/>,
  },
];

const PumpsWidget: React.FC<PumpsWidgetProps> = (props) => {
  const [selectedPump, setSelectedPump] = useState<PumpData>(PUMPS[0]);
  const { isOpen, onOpen, onOpenChange } = useDisclosure();

  const [defaultDurations, _] = useGenericStore<Record<string, number>>("pumpDefaultDurations");
  const [duration, setDuration] = useState<string>(defaultDurations[selectedPump.value]?.toString() ?? "10");

  const bifrostRun = useBifrost({ service: RosService.PUMPS_RUN });
  const bifrostStop = useBifrost({ service: RosService.PUMPS_STOP });
  const bifrostStatus = useBifrost({ topic: RosTopic.PUMPS_STATUS });

  const pumpStatus = useSelector((state: RootState) => state.pumpsStatusStore);

  useEffect(() => {
    bifrostStatus.syncWithTopic();
  }, [bifrostStatus]);

  // Prefill duration when pump selection changes
  const onSelectedPumpChange = useCallback((keys: SharedSelection) => {
    // Update selectedPump state
    const selected = Array.from(keys)[0] as string;
    const pump = PUMPS.find(p => p.value === selected) ?? PUMPS[0];
    if (selected) setSelectedPump(pump);

    // Update the duration state based on the selected pump's default duration
    const defaultDuration = defaultDurations[pump.value];
    if (defaultDuration !== undefined) {
      setDuration(defaultDuration.toString());
    }
  }, [defaultDurations, setSelectedPump]);

  const runPump = () => {
    const durationNum = Number(duration);
    const request = {
      pump: selectedPump.value.replace('/prime', ''),
      duration: isNaN(durationNum) || durationNum <= 0 ? 0 : durationNum,
    };

    bifrostRun.callService(request, {
      responseToast: true,
      successToastMessage: `Started pump: ${selectedPump.display}`,
    });
  };

  const stopPump = () => {
    bifrostStop.callService({}, {
      responseToast: true,
      successToastMessage: "Pump stopped",
    });
  };

  const timeField = (
    <Input
      className="col-span-2"
      label="Duration"
      placeholder="0.0s"
      value={duration}
      onValueChange={setDuration}
      isDisabled={pumpStatus.running}
      endContent={
        <div className="pointer-events-none flex items-center">
          <span className="text-default-400 text-small">s</span>
        </div>
      }
    />
  );

  const progressBar = (
    <div className="flex flex-row items-center justify-center">
      {selectedPump.leftIcon}
      <Progress
        disableAnimation
        color="secondary"
        aria-label="Pump Progress"
        value={pumpStatus.running ? pumpStatus.time_elapsed : 0}
        maxValue={pumpStatus.running && pumpStatus.time_target > 0 ? pumpStatus.time_target : 1}
      />
      {selectedPump.rightIcon}
    </div>
  );

  const getSelectedPumpDisplay = () => {
    return PUMPS.find(p => p.value === pumpStatus.pump)?.display || pumpStatus.pump;
  };

  return (
    <Card {...props}>
      <CardHeader className="pb-0 flex flex-row justify-between items-center">
        <div className="flex-1">Pumps</div>
        <div className="flex-1 text-center text-sm text-default-500">
          {pumpStatus.running && `Running: ${getSelectedPumpDisplay()}`}
        </div>
        <div className="flex-1 flex justify-end">
          <Button isIconOnly variant="light" size="sm" onPress={onOpen}>
            <MoreHorizontal/>
          </Button>
        </div>
      </CardHeader>
      <CardBody className="flex flex-col">
        {progressBar}

        <div className="flex flex-row justify-around">
          <div className="w-40 text-center">
            <span>
              {pumpStatus.running
                ? `${pumpStatus.time_elapsed.toFixed(2)} / ${pumpStatus.time_target.toFixed(2)}s`
                : `0 / ${Number(duration)}s`}
            </span>
          </div>
        </div>

        <div className="grid grid-cols-9 gap-3 mt-3">
          <Select
            label="Select Pump"
            className="col-span-3"
            selectedKeys={[selectedPump.value]}
            onSelectionChange={onSelectedPumpChange}
            isDisabled={pumpStatus.running}
          >
            {PUMPS.map((pump) => (
              <SelectItem key={pump.value}>{pump.display}</SelectItem>
            ))}
          </Select>
          {timeField}

          <div className="grid grid-cols-2 gap-3 justify-center items-center col-span-4">
            <Button color="primary" isDisabled={pumpStatus.running} onPress={runPump}>
              Run
            </Button>
            <Button color="danger" onPress={stopPump}>
              Cancel
            </Button>
          </div>
        </div>
      </CardBody>
      <PumpsModal isOpen={isOpen} onOpenChange={onOpenChange}/>
    </Card>
  );
};

export default PumpsWidget;