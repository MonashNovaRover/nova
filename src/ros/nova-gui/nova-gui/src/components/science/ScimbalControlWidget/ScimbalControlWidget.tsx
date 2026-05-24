import React, { useCallback, useMemo } from "react";
import {
  Button,
  Card,
  CardBody,
  CardHeader,
  Input,
  Popover,
  PopoverContent,
  PopoverTrigger,
} from "@nextui-org/react";
import { ChevronUp, ChevronDown, ChevronLeft, ChevronRight, MoreHorizontal } from "react-feather";
import toast from "react-hot-toast";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { useGenericStore } from "../../../hooks/useGenericStore.ts";

const SCIMBAL_TOAST_ID = "scimbal-control";

/**
 * Scimbal Camera Control Widget
 * Provides arrow button controls for pan/tilt of the scimbal camera.
 * Supports keyboard arrow key control when focused.
 */
const ScimbalControlWidget: React.FC = () => {
  const [step, setStep] = useGenericStore<string>("scimbalStepSize");
  const stepNumber = useMemo(() => {
    const val = parseInt(step);
    if (isNaN(val) || val <= 0) return 1;
    return val;
  }, [step]);

  const bifrost = useBifrost({ service: RosService.SCIMBAL_COMMAND });

  // Tilt (up/down): angles[0], Pan (left/right): angles[1]
  const incrementTilt = useCallback(
    (delta: number) =>
      bifrost.callService(
        { angles: [delta, 0] },
        {
          responseToast: false,
          handleResponse: () =>
            toast.success(`Scimbal Cam moved ${delta > 0 ? "+" : ""}${delta}° tilt`, { id: SCIMBAL_TOAST_ID }),
        }
      ),
    [bifrost]
  );

  const incrementPan = useCallback(
    (delta: number) =>
      bifrost.callService(
        { angles: [0, delta] },
        {
          responseToast: false,
          handleResponse: () =>
            toast.success(`Scimbal Cam moved ${delta > 0 ? "+" : ""}${delta}° pan`, { id: SCIMBAL_TOAST_ID }),
        }
      ),
    [bifrost]
  );

  const handleKeyDown = useCallback(
    (e: React.KeyboardEvent<HTMLDivElement>) => {
      switch (e.key) {
        case "ArrowUp":
          incrementTilt(stepNumber);
          e.preventDefault();
          break;
        case "ArrowDown":
          incrementTilt(-stepNumber);
          e.preventDefault();
          break;
        case "ArrowLeft":
          incrementPan(-stepNumber);
          e.preventDefault();
          break;
        case "ArrowRight":
          incrementPan(stepNumber);
          e.preventDefault();
          break;
      }
    },
    [incrementTilt, incrementPan, stepNumber]
  );

  return (
    <Card>
      <CardHeader className="pb-0 flex flex-row justify-between items-center">
        <span>Scimbal Cam</span>
        <Popover placement="bottom-end">
          <PopoverTrigger>
            <Button isIconOnly variant="light" size="sm">
              <MoreHorizontal size={18} />
            </Button>
          </PopoverTrigger>
          <PopoverContent className="dark text-foreground">
            <div className="px-2 py-2 w-48">
              <Input
                label="Step size"
                type="number"
                size="sm"
                value={step}
                onValueChange={setStep}
              />
            </div>
          </PopoverContent>
        </Popover>
      </CardHeader>
      <CardBody>
        <div
          tabIndex={0}
          onKeyDown={handleKeyDown}
          className="flex flex-col items-center gap-2 outline-none focus:ring-2 focus:ring-primary rounded-lg p-2"
        >
          {/* Up button */}
          <Button
            isIconOnly
            size="lg"
            variant="flat"
            onPress={() => incrementPan(-stepNumber)}
            aria-label="Tilt up"
          >
            <ChevronUp size={28} />
          </Button>

          {/* Left, center, Right row */}
          <div className="flex flex-row items-center gap-2">
            <Button
              isIconOnly
              size="lg"
              variant="flat"
              onPress={() => incrementTilt(-stepNumber)}
              aria-label="Pan left"
            >
              <ChevronLeft size={28} />
            </Button>

            {/* Center indicator */}
            <div className="w-12 h-12 flex items-center justify-center">
              <div className="w-3 h-3 rounded-full bg-default-400" />
            </div>

            <Button
              isIconOnly
              size="lg"
              variant="flat"
              onPress={() => incrementTilt(stepNumber)}
              aria-label="Pan right"
            >
              <ChevronRight size={28} />
            </Button>
          </div>

          {/* Down button */}
          <Button
            isIconOnly
            size="lg"
            variant="flat"
            onPress={() => incrementPan(stepNumber)}
            aria-label="Tilt down"
          >
            <ChevronDown size={28} />
          </Button>
        </div>
      </CardBody>
    </Card>
  );
};

export default ScimbalControlWidget;
