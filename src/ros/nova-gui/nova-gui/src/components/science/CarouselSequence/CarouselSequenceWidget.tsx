import React, { useState } from "react";
import {
  Button,
  Card,
  CardBody,
  CardHeader,
  CardProps,
  Input,
  Progress,
  Select,
  SelectItem,
} from "@nextui-org/react";
import { Play, Square, AlertCircle } from "react-feather";
import {
  useCarouselSequenceStatus,
  useCarouselSequenceControl,
} from "./useCarouselSequenceBifrost.ts";

export interface CarouselSequenceWidgetProps extends CardProps {}

const STEP_LABELS: Record<string, string> = {
  idle: "Idle",
  pumping: "Pumping",
  moving: "Moving Carousel",
  delay_pump: "Waiting (post-pump)",
  delay_move: "Waiting (post-move)",
  error: "Error",
  complete: "Complete",
};

/**
 * Widget to control the automated carousel fill sequence.
 */
const CarouselSequenceWidget: React.FC<CarouselSequenceWidgetProps> = (props) => {
  const status = useCarouselSequenceStatus();
  const { startSequence, stopSequence } = useCarouselSequenceControl();

  // Form state
  const [ring, setRing] = useState<"inner" | "outer">("inner");
  const [iterations, setIterations] = useState<number>(6);
  const [pumpDuration, setPumpDuration] = useState<number>(2.0);

  const isRunning = status.running;
  const isComplete = status.current_step === "complete";
  const isError = status.current_step === "error";

  const handleStart = () => {
    startSequence(ring, iterations, pumpDuration);
  };

  const handleStop = () => {
    stopSequence();
  };

  const progressPercent =
    status.total_iterations > 0
      ? (status.current_iteration / status.total_iterations) * 100
      : 0;

  return (
    <Card {...props}>
      <CardHeader className="pb-0 flex flex-row items-center justify-between gap-2">
        <span>Carousel Fill Sequence</span>
        {isRunning && (
          <span className="text-sm text-primary animate-pulse">Running...</span>
        )}
        {isComplete && (
          <span className="text-sm text-success">Complete</span>
        )}
        {isError && (
          <span className="text-sm text-danger flex items-center gap-1">
            <AlertCircle size={14} /> Error
          </span>
        )}
      </CardHeader>
      <CardBody className="flex flex-col gap-4">
        {/* Configuration Section */}
        <div className="grid grid-cols-3 gap-3">
          <Select
            label="Ring"
            selectedKeys={[ring]}
            onSelectionChange={(keys) => {
              const selected = Array.from(keys)[0] as "inner" | "outer";
              if (selected) setRing(selected);
            }}
            isDisabled={isRunning}
            size="sm"
          >
            <SelectItem key="inner">Inner</SelectItem>
            <SelectItem key="outer">Outer</SelectItem>
          </Select>

          <Input
            label="Iterations"
            type="number"
            value={iterations.toString()}
            onValueChange={(val) => setIterations(parseInt(val) || 1)}
            isDisabled={isRunning}
            size="sm"
            min={1}
            max={24}
          />

          <Input
            label="Pump (sec)"
            type="number"
            value={pumpDuration.toString()}
            onValueChange={(val) => setPumpDuration(parseFloat(val) || 1.0)}
            isDisabled={isRunning}
            size="sm"
            min={0.5}
            step={0.5}
          />
        </div>

        {/* Control Buttons */}
        <div className="grid grid-cols-2 gap-3">
          <Button
            color="success"
            onPress={handleStart}
            isDisabled={isRunning}
            startContent={<Play size={16} />}
          >
            Start Sequence
          </Button>
          <Button
            color="danger"
            onPress={handleStop}
            isDisabled={!isRunning}
            startContent={<Square size={16} />}
          >
            Stop
          </Button>
        </div>

        {/* Status Section */}
        {(isRunning || isComplete || isError) && (
          <div className="flex flex-col gap-2">
            <div className="flex justify-between text-sm">
              <span>
                Step: <span className="font-medium">{STEP_LABELS[status.current_step] || status.current_step}</span>
              </span>
              <span>
                Iteration: {status.current_iteration} / {status.total_iterations}
              </span>
            </div>

            <Progress
              value={progressPercent}
              color={isError ? "danger" : isComplete ? "success" : "primary"}
              size="sm"
              showValueLabel
              className="w-full"
            />

            {status.ring && (
              <span className="text-xs text-default-500">
                Ring: {status.ring.charAt(0).toUpperCase() + status.ring.slice(1)}
              </span>
            )}

            {isError && status.error_message && (
              <div className="text-sm text-danger bg-danger-50 p-2 rounded">
                {status.error_message}
              </div>
            )}
          </div>
        )}
      </CardBody>
    </Card>
  );
};

export default CarouselSequenceWidget;
