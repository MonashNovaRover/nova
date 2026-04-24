import { Card, CardBody, CardHeader, Switch} from "@nextui-org/react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { IRosScienceInterfacesToggleRequest} from "../../../ros/rosTypes.ts";

interface LED {
  displayName: string
  name: string
}

const LED_CONFIG: LED[] = [
  { name: "vis_spec_central", displayName: "Vis Spec Central" },
  { name: "vis_spec_nile_red", displayName: "Vis Spec Nile Red" },
  { name: "vis_spec_camera", displayName: "Vis Spec Camera" },
  { name: "vis_spec_nadh", displayName: "Vis Spec NADH" },
  { name: "litmus_led", displayName: "Litmus LED" },
]

const Leds = () => {
    const bifrost = useBifrost({ service: RosService.LEDS});

    const handleLedToggle= (led_name:string)=> {
        const requestPayload: IRosScienceInterfacesToggleRequest = {
            name: led_name,
        };

        bifrost.callService(
            requestPayload,
            {
                responseToast: true,
                successToastMessage: `Led ${led_name} toggle successful`,
                errorToastMessage: `Failed to toggle ${led_name}`,
            }
        )
    };

    return (
      <Card>
        <CardHeader>
          Vis Spec LEDs
        </CardHeader>
        <CardBody className="grid grid-cols-5 justify-between">
          {LED_CONFIG.map((led: LED) => (
            <div className="flex flex-col gap-2 items-center">
              <span>{led.displayName}</span>
              <Switch
                size="lg"
              />
            </div>
          ))}
        </CardBody>
      </Card>
    );
};

export default Leds;