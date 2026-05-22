import { Card, CardBody, CardHeader, Switch} from "@nextui-org/react";
import { useState } from "react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../ros/services/rosService.ts";

interface LED {
  displayName: string
  name: string
}

const LED_CONFIG: LED[] = [
  { name: "vis_spec_central_led", displayName: "Spec Central" },
  { name: "vis_spec_nile_red", displayName: "Spec Nile Red" },
  { name: "vis_spec_camera", displayName: "Spec Roof" },
  { name: "vis_spec_nadh", displayName: "Spec UV" },
]

const LEDWidget = () => {
    const bifrost = useBifrost({ service: RosService.LEDS});
    const [ledStates, setLedStates] = useState<Record<string, boolean>>({});

    const handleLedChange = (led_name: string, value: boolean) => {
        const requestPayload = {
            name: led_name,
            value: value,
        };

        bifrost.callService(
            requestPayload,
            {
                responseToast: true,
                successToastMessage: `LED ${led_name} request successful`,
                errorToastMessage: `Failed to set ${led_name}`,
                // update if successful
                handleResponse: (response) => {
                  if (response && 'success' in response && response.success) {
                    setLedStates(prev => ({ ...prev, [led_name]: value }));
                  }
                }
            }
        );
    };

    return (
      <Card>
        <CardHeader>
          Vis Spec LEDs
        </CardHeader>
        <CardBody className="grid grid-cols-4 justify-between">
          {LED_CONFIG.map((led: LED) => (
            <div key={led.name} className="flex flex-col gap-2 items-center">
              <span className="text-sm">{led.displayName}</span>
              <Switch
                size="lg"
                isSelected={ledStates[led.name] || false}
                onValueChange={(value) => handleLedChange(led.name, value)}
              />
            </div>
          ))}
        </CardBody>
      </Card>
    );
};

export default LEDWidget;