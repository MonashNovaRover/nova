import React, {useEffect} from "react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {Card, CardBody, CardHeader, CardProps, Divider} from "@nextui-org/react";
import {RosService} from "../../../ros/services/rosService.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import NTCData from "./NTCData.tsx";
import HeaterToggle from "./HeaterToggle.tsx";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";

export interface HeaterWidgetWidgetProps extends CardProps {
}

const HeaterWidget: React.FC<HeaterWidgetWidgetProps> = (props) => {
  const bifrost = useBifrost({topic:RosTopic.KILN_DATA, service: RosService.HEATER});
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
    <CardHeader>NTC Temperature Sensors</CardHeader>
    <CardBody>
      <NTCData tempReadings={tempReadings.temp} labels={["Heater", "Dirt"]} suffixes={["°C", "°C"]}/>
    </CardBody>
    <div className="m-3">
      <Divider/>
    </div>
    <CardHeader>Heater</CardHeader>
    <CardBody>
      <HeaterToggle
        currentHeaterStatus={tempReadings.state}
        setHeaterStatus={setHeaterStatus}
        targetTemp={targetTemp}
        setTargetTemp={updateTargetTemp}
      />
    </CardBody>
  </Card>
}

export default HeaterWidget