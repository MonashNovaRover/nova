import React, { useEffect } from "react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { Card, CardBody, CardProps } from "@nextui-org/react";
import { RosService } from "../../../ros/services/rosService.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import HeaterControl from "../HeaterWidget/HeaterControl.tsx";
import { useGenericStore } from "../../../hooks/useGenericStore.ts";
import KilnChart from "./KilnChart.tsx";

export interface KilnWidgetWidgetProps extends CardProps {
}

/**
 * Kiln controls widget.
 * @param props
 * @constructor
 */
const KilnWidget: React.FC<KilnWidgetWidgetProps> = (props) => {
    const bifrost = useBifrost({ topic: RosTopic.KILN_DATA, service: RosService.KILN_COMMAND });
    const tempReadings = useSelector((state: RootState) => state.kilnData);
    const [targetTemp, setTargetTemp] = useGenericStore<number>("targetTemp");

    const sendCommand = (state: boolean, temp: number) => bifrost.callService({ state: state, target: temp });

    useEffect(() => {
        bifrost.syncWithTopic();
    }, [bifrost]);

    const updateTargetTemp = (temp: number) => {
        setTargetTemp(temp)
        sendCommand(tempReadings.state, temp)
    }

    const setKilnStatus = (state: boolean) => sendCommand(state, targetTemp)

    return <Card {...props}>
        <CardBody className="flex flex-col gap-3">
            <HeaterControl
                heaterName="Kiln"
                currentHeaterStatus={tempReadings.state}
                setHeaterStatus={setKilnStatus}
                targetTemp={targetTemp}
                setTargetTemp={updateTargetTemp}
            />
            <KilnChart></KilnChart>
        </CardBody>
    </Card>
}

export default KilnWidget
