import { Button} from "@nextui-org/react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RosService } from "../../../ros/services/rosService.ts";
import { IRosScienceInterfacesToggleRequest} from "../../../ros/rosTypes.ts";

const Leds = () => {
    const bifrost = useBifrost({ service: RosService.LEDS});

    const handleLedToggle=(led_name:string)=>{
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
        <div>
            <Button
            onClick = {()=> handleLedToggle("vis_spec_central_led")}
            >
                Vis spec central 
            </Button>

        </div>

    );
};

export default Leds;