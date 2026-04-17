import React, {useEffect} from "react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {Card, CardBody, CardHeader, CardProps, Divider} from "@nextui-org/react";
import {RosService} from "../../../ros/services/rosService.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import SensorDataDisplay from "../SensorDataDisplay.tsx";
import ThermalControl from "./ThermalControl.tsx";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";

export interface HeaterWidgetWidgetProps extends CardProps {
}

/**
 * Heater controls and temperature sensor data widget.
 * @param props
 * @constructor
 */
const HeaterWidget: React.FC<HeaterWidgetWidgetProps> = (props) => {
  const bifrost = useBifrost({topic:RosTopic.KILN_DATA, service: RosService.KILN_COMMAND});
  const tempReadings = useSelector((state: RootState) => state.kilnData);
  const [targetTemp, setTargetTemp] = useGenericStore<number>("targetTemp");

  const sendCommand = (state: boolean, temp: number) => bifrost.callService({state: state, target: temp});

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const updateTargetTemp = (temp: number) => {
    setTargetTemp(temp)
    sendCommand(tempReadings.state, temp)
  }

  const setHeaterStatus = (state: boolean) => sendCommand(state, targetTemp)

  return <Card {...props}>
    <CardHeader className="pb-0">Temperature Sensors</CardHeader>
    <CardBody>
      <SensorDataDisplay values={tempReadings.temp} labels={["Heater", "Dirt"]} suffixes={["°C", "°C"]}/>
    </CardBody>
    <div className="mx-3 my-1">
      <Divider/>
    </div>
    <CardBody>
      <ThermalControl
        currentHeaterStatus={tempReadings.state}
        setHeaterStatus={setHeaterStatus}
        targetTemp={targetTemp}
        setTargetTemp={updateTargetTemp}
      />
    </CardBody>
  </Card>
}

export default HeaterWidget