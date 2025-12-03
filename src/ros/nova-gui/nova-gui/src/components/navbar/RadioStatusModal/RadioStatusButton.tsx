import { Button } from "@nextui-org/react";
import { RadioStatusModal } from "./RadioStatusModal.tsx";
import { Radio } from "react-feather";
import { useUIActions } from "../../../redux/actions/useUIActions.ts";
import { useRadioMonitor } from "./hooks/useRadioMonitor.ts";

export function RadioStatusButton() {

    const uiActions = useUIActions();
    const radioHealth = useRadioMonitor();

    return (
        <div>
            <Button
                size="sm"
                variant="solid"
                color={"default"}
                onPress={() => uiActions.setRadioStatusModalOpen(true)}
            >
                <Radio className="w-4 h-4" />
            {radioHealth}
            </Button>
            <RadioStatusModal/>
        </div>
    );

}
