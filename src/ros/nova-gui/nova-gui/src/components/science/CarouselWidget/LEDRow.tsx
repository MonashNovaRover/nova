import React, {useState} from "react";
import {Switch} from "@nextui-org/react";
import {IRosStdSrvsSetBoolResponse} from "../../../ros/rosTypes.ts";
import toast from "react-hot-toast";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";

interface LEDRowProps {
}

const LEDRow: React.FC<LEDRowProps> = () => {
  const [ LED1, setLED1 ] = useState<boolean>(false);
  const [ LED2, setLED2 ] = useState<boolean>(false);

  const bifrostLed1 = useBifrost({service: RosService.UV_VIS_LED_1});
  const bifrostLed2 = useBifrost({service: RosService.UV_VIS_LED_2});

  const changeLED1 = (value: boolean) => {
    bifrostLed1.callService({data: value}, {
      handleResponse: (response) => {
        const boolResponse = response as IRosStdSrvsSetBoolResponse;
        if (boolResponse?.success) {
          setLED1(value);
        }
        else{
          toast.error("Failed to switch LED 1")
        }
      },
      sendToRedux: true,
    });
  }

  const changeLED2 = (value: boolean) => {
    bifrostLed2.callService({data: value}, {
      handleResponse: (response) => {
        const boolResponse = response as IRosStdSrvsSetBoolResponse;
        if (boolResponse?.success) {
          setLED2(value);
        }
        else{
          toast.error("Failed to switch LED 2")
        }
      },
      sendToRedux: true,
    });
  }

  return (
    <div className="flex flex-row gap-5">
      <div>
        <div className="font-bold">LED 1 (T)</div>
        <Switch
          className="mt-1.5 mx-1.5"
          isSelected={LED1}
          onChange={() => changeLED1(!LED1)}
        />
      </div>
      <div>
        <div className="font-bold">LED 2 (B)</div>
        <Switch
          className="mt-1.5 mx-1.5"
          isSelected={LED2}
          onChange={() => changeLED2(!LED2)}
        />
      </div>
    </div>
  );
}

export default LEDRow;
