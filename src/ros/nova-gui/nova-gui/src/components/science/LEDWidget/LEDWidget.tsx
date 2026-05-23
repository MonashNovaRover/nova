import { Card, CardBody, CardHeader, Switch} from "@nextui-org/react";
import { useEffect } from "react";
import { useSelector } from "react-redux";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { RootState } from "../../../redux/RootState.ts";

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
    const bifrostService = useBifrost({ service: RosService.LEDS });
    const bifrostTopic = useBifrost({ topic: RosTopic.LED_STATUS });

    // Subscribe to LED status topic
    const ledStatus = useSelector((state: RootState) => state.ledStatusStore);

    useEffect(() => {
      bifrostTopic.syncWithTopic();
    }, [bifrostTopic]);

    // Convert arrays to lookup object for easy access
    const ledStates: Record<string, boolean> = {};
    ledStatus.names.forEach((name, i) => {
      ledStates[name] = ledStatus.values[i];
    });

    const handleLedChange = (led_name: string, value: boolean) => {
        bifrostService.callService(
            { name: led_name, value: value },
            {
                responseToast: true,
                successToastMessage: `LED ${led_name} request successful`,
                errorToastMessage: `Failed to set ${led_name}`,
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
