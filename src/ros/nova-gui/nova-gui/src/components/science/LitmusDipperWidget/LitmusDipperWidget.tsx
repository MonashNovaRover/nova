import {
  Button, Card, CardBody, CardHeader, CardProps, Input, Progress,
  useDisclosure
} from "@nextui-org/react";
import React, { useEffect, useState } from "react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { ArrowDown, ArrowUp, MoreHorizontal } from "react-feather";
import { useGenericStore } from "../../../hooks/useGenericStore.ts";
import LitmusDipperModal from "./LitmusDipperModal.tsx";

export interface LitmusDipperWidgetProps extends CardProps { }

export interface LitmusDipperConfig {
  defaultDuration: number;
  twitchStep: number;
  waitDuration: number;
}

/**
 * Litmus dipper widget (repurposed from PumpsWidget.tsx)
 * @param props
 * @constructor
 */
const LitmusDipperWidget: React.FC<LitmusDipperWidgetProps> = (props) => {
  const { isOpen, onOpen, onOpenChange } = useDisclosure();

  const [config, setConfig] = useGenericStore<LitmusDipperConfig>("litmusDipperConfig");
  const defaultDuration = config?.defaultDuration ?? 2;
  const twitchStep = config?.twitchStep ?? 5;
  const waitDuration = config?.waitDuration ?? 30;

  const [twitchInput, setTwitchInput] = useState(twitchStep?.toString());
  const [duration, setDuration] = useState<string>(defaultDuration?.toString());

  const [waitingForReady, setWaitingForReady] = useState(false);
  const [isReady, setIsReady] = useState(false);
  const [waitTimeElapsed, setWaitTimeElapsed] = useState(0);
  const [wasRunning, setWasRunning] = useState(false);

  const bifrostDip = useBifrost({ service: RosService.LITMUS_DIPPER_DIP });
  const bifrostStop = useBifrost({ service: RosService.LITMUS_DIPPER_STOP });
  const bifrostTwitch = useBifrost({ service: RosService.LITMUS_DIPPER_TWITCH });
  const bifrostStatus = useBifrost({ topic: RosTopic.LITMUS_DIPPER_STATUS });

  const dipperStatus = useSelector((state: RootState) => state.litmusDipperStatusStore);

  useEffect(() => {
    bifrostStatus.syncWithTopic();
  }, [bifrostStatus]);

  useEffect(() => {
    if (wasRunning && !dipperStatus.running) {
      setWaitingForReady(true);
      setWaitTimeElapsed(0);
      setIsReady(false);
    }
    setWasRunning(dipperStatus.running);
  }, [dipperStatus.running, wasRunning]);

  useEffect(() => {
    if (!waitingForReady) return;

    const interval = setInterval(() => {
      setWaitTimeElapsed((prev) => {
        if (prev >= waitDuration) {
          setWaitingForReady(false);
          setIsReady(true);
          return waitDuration;
        }
        return prev + 0.1;
      });
    }, 100);

    return () => clearInterval(interval);
  }, [waitingForReady, waitDuration]);

  const runDip = () => {
    setIsReady(false);
    setWaitingForReady(false);
    const durationNum = Number(duration);
    bifrostDip.callService({
      pump: "litmus_dipper",
      duration: isNaN(durationNum) || durationNum <= 0 ? 2 : durationNum,
    }, {
      responseToast: true,
      successToastMessage: `Dipping for ${duration}s`,
    });
  };

  const stopDip = () => {
    bifrostStop.callService({}, {
      responseToast: true,
      successToastMessage: "Dip stopped",
    });
  };

  const twitch = (direction: "up" | "down") => {
    const offset = direction === "up" ? -(twitchStep ?? 5) : (twitchStep ?? 5);
    bifrostTwitch.callService({ position: offset });
  };

  const progressBar = (
    <div className="flex flex-row items-center gap-3">
      <Progress
        className="flex-1"
        disableAnimation
        color={waitingForReady || isReady ? "success" : "secondary"}
        aria-label="Dip Progress"
        value={
          isReady ? 100 :
          waitingForReady ? waitTimeElapsed :
          dipperStatus.running ? dipperStatus.time_elapsed : 0
        }
        maxValue={
          isReady ? 100 :
          waitingForReady ? waitDuration :
          dipperStatus.running && dipperStatus.time_target > 0 ? dipperStatus.time_target : 1
        }
      />
      <span className="whitespace-nowrap w-24 text-center">
        {isReady
          ? "Ready!"
          : waitingForReady
          ? `${waitTimeElapsed.toFixed(1)} / ${waitDuration}s`
          : dipperStatus.running
          ? `${dipperStatus.time_elapsed.toFixed(1)} / ${dipperStatus.time_target.toFixed(1)}s`
          : `0 / ${Number(duration)}s`}
      </span>
    </div>
  );

  return (
    <Card {...props} className={`${props.className ?? ""} ${isReady ? "border-2 border-success" : ""}`}>
      <CardHeader className="pb-0 flex flex-row justify-between items-center">
        <div className="flex-1">Litmus Dipper</div>
        <div className="flex-1 text-center text-sm text-default-500">
          {dipperStatus.running && "Dipping..."}
        </div>
        <div className="flex-1 flex justify-end">
          <Button isIconOnly variant="light" size="sm" onPress={onOpen}>
            <MoreHorizontal />
          </Button>
        </div>
      </CardHeader>
      <CardBody className="flex flex-col">
        {progressBar}
        <div className="flex flex-col gap-3 mt-3">
          <div className="flex flex-row gap-3">
            <Input
              className="w-1/2"
              label="Duration"
              placeholder="2.0s"
              value={duration}
              onValueChange={setDuration}
              isDisabled={dipperStatus.running}
              endContent={
                <div className="pointer-events-none flex items-center">
                  <span className="text-default-400 text-small">s</span>
                </div>
              }
            />
            <Input
              className="w-1/2"
              label="Twitch Step"
              type="number"
              value={twitchInput}
              onValueChange={(v) => {
                setTwitchInput(v);
                const n = parseFloat(v);
                if (!isNaN(n) && n > 0) setConfig({ ...config, twitchStep: n });
              }}
              endContent={
                <div className="pointer-events-none flex items-center">
                  <span className="text-default-400 text-small">°</span>
                </div>
              }
              isDisabled={dipperStatus.running}
            />
          </div>
          <div className="flex flex-row justify-between gap-3">
            <Button className="w-1/4" color="primary" isDisabled={dipperStatus.running} onPress={runDip}>
              Dip
            </Button>
            <Button className="w-1/4" color="danger" onPress={stopDip}>
              Cancel
            </Button>
            <Button className="w-1/4" onPressStart={() => twitch("up")} isDisabled={dipperStatus.running}>
              <ArrowUp />
            </Button>
            <Button className="w-1/4" onPressStart={() => twitch("down")} isDisabled={dipperStatus.running}>
              <ArrowDown />
            </Button>
          </div>
        </div>
      </CardBody>
      <LitmusDipperModal isOpen={isOpen} onOpenChange={onOpenChange} />
    </Card>
  );
};

export default LitmusDipperWidget;