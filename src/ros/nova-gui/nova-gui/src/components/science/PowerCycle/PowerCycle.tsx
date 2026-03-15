import { useState } from "react";
import { Button, Input } from "@nextui-org/react";
import { Zap } from "react-feather";
import toast from "react-hot-toast";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { IRosScienceInterfacesPowerCycleRequest } from "../../../ros/rosTypes.ts";

const SciencePowerCycle = () => {
  const [sleepTime, setSleepTime] = useState<string>("1.0");
  const [isLoading, setIsLoading] = useState<boolean>(false);
  const powerCycleScience = useBifrost({service: RosService.POWER_CYCLE_SCIENCE });
  const powerCycleDrive = useBifrost({service: RosService.POWER_CYCLE_DRIVE });

  const handlePowerCycle = (service: ReturnType<typeof useBifrost>) => {
    const duration = parseFloat(sleepTime);
    if (isNaN(duration) || duration <= 0) {
      toast.error("Enter a valid sleep duration:");
      return;
    }

    setIsLoading(true);

    const requestPayload: IRosScienceInterfacesPowerCycleRequest = {
      sleep_duration: duration,
    };

    service.callService(
      requestPayload,
      {
        responseToast: true,
        successToastMessage: `Power cycle successful`,
        errorToastMessage: `Failed to power cycle`,
        handleResponse: () => setIsLoading(false),
      }
    )
  };

  return (
    <div className="grid grid-cols-2 items-end gap-3">
      <Input
        type="number"
        label="Sleep Time"
        placeholder="1.0"
        endContent={<div className="text-small text-default-400">sec</div>}
        value={sleepTime}
        onChange={(e) => setSleepTime(e.target.value)}
        size="sm"
        min="0.1"
        step="0.1"
      />

      <div className="flex flex-col gap-3">
        <Button 
          color="warning" 
          variant="shadow"
          size="lg"
          isLoading={isLoading} 
          onPressStart={()=>handlePowerCycle(powerCycleScience)}
          startContent={!isLoading && <Zap size={18} />}
        >
          Cycle Science Rails
        </Button>

        <Button 
          color="warning" 
          variant="shadow"
          size="lg"
          isLoading={isLoading} 
          onPressStart={()=>handlePowerCycle(powerCycleDrive)}
          startContent={!isLoading && <Zap size={18} />}
        >
          Cycle Drive Rails
        </Button>
      </div>
        
    </div>
  );
};

export default SciencePowerCycle;