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
  const bifrost = useBifrost({ service: RosService.POWER_CYCLE_SCIENCE });

  const handlePowerCycle = async () => {
    const duration = parseFloat(sleepTime);
    if (isNaN(duration) || duration <= 0) {
      toast.error("Enter a valid sleep duration:");
      return;
    }

    setIsLoading(true);
    const toastId = toast.loading('Power cycling');

    try {
      const requestPayload: IRosScienceInterfacesPowerCycleRequest = {
        sleep_duration: duration,
      };

      await bifrost.callService(requestPayload);
      toast.success('Power cycle complete!', { id: toastId });
      
    } catch (error) {
      console.error("Bifrost Service Error:", error);
      toast.error('Failed to reach PC2 controller.', { id: toastId });
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="flex flex-row items-end gap-3 p-4 bg-gray-800 rounded-xl w-max border border-gray-700">
      <Input
        type="number"
        label="Sleep Time"
        placeholder="1.0"
        endContent={<div className="text-small text-default-400">sec</div>}
        value={sleepTime}
        onChange={(e) => setSleepTime(e.target.value)}
        className="w-28"
        size="sm"
        min="0.1"
        step="0.1"
      />
      <Button 
        color="warning" 
        variant="shadow"
        isLoading={isLoading} 
        onClick={handlePowerCycle}
        startContent={!isLoading && <Zap size={18} />}
      >
        Cycle Rails
      </Button>
    </div>
  );
};

export default SciencePowerCycle;