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

  const handlePowerCycle = () => {
    const duration = parseFloat(sleepTime);
    if (isNaN(duration) || duration <= 0) {
      toast.error("Enter a valid sleep duration:");
      return;
    }

    setIsLoading(true);
    // const toastId = toast.loading('Power cycling');

    const requestPayload: IRosScienceInterfacesPowerCycleRequest = {
      sleep_duration: duration,
    };

    // bifrost.callService(requestPayload);
    bifrost.callService(
      requestPayload,
      {
        responseToast: true,
        successToastMessage: `Power cycle successful`,
        errorToastMessage: `Failed to power cycle`,
        handleResponse: () => setIsLoading(false),
      }
    )
      // toast.success('Power cycle complete!', { id: toastId });
      
    // } catch (error) {
    //   console.error("Bifrost Service Error:", error);
    //   toast.error('Failed to reach PC2 controller.', { id: toastId });
    // } finally {
    //   setIsLoading(false);
    // }
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
      <Button 
        color="warning" 
        variant="shadow"
        size="lg"
        isLoading={isLoading} 
        onPressStart={handlePowerCycle}
        startContent={!isLoading && <Zap size={18} />}
      >
        Cycle Rails
      </Button>
    </div>
  );
};

export default SciencePowerCycle;