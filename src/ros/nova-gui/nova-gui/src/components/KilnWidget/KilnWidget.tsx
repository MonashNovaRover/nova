import React, { useState, useEffect } from "react";
import {Button, Card, CardHeader, CardBody, CardProps, Slider, Tooltip} from "@nextui-org/react";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../ros/topics/rosTopic";
import { RosService } from "../../ros/services/rosService";
import { SubCardLabel } from "../shared/Labels";
import { Square, Power } from "react-feather";
import { OverlayedProgress } from "../OverlayedProgress/OverlayedProgress";

interface KilnWidgetProps extends CardProps {

}


const KilnWidget: React.FC<KilnWidgetProps> = (props) => {
  const [maxTemp, setMaxTemp] = useState(130);
  const [goalTemp, setGoalTemp] = useState<number>(25);
  const [inputGoalTemp, setInputGoalTemp] = useState<string>("25");

  const handleChange = (goalTemp) => {
    if (isNaN(Number(goalTemp))) return;

    setGoalTemp(goalTemp);
    setInputGoalTemp(goalTemp.toString());
  };

  // set up to 'refresh' kilnData state
  const kilnData = useSelector(
    (state: RootState) => state.kilnData
  );

  const kilnServiceData = useSelector(
    (state: RootState) => state.kilnCommand
  );

  const dataBifrost = useBifrost({topic: RosTopic.KILN_DATA});
  const serviceBifrost = useBifrost({service: RosService.KILN_COMMAND});
  //const serviceBifrost = useBifrost({service: RosService.KILN_COMMAND});
  const toggleKilnState = () => serviceBifrost.callServiceToRedux({state: !kilnData.state});
  const sendTarget = () => serviceBifrost.callServiceToRedux({target: goalTemp});

  useEffect(() => {
    dataBifrost.syncWithTopic(); // calling ros bridge to subscribe to topic
    // update max temps if current temps exceed them
    if (kilnData.temp[0] > maxTemp) {
      const result = maxTemp;
      result[0] = 1.1 * kilnData.temp[0];
      setMaxTemp(result);
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [dataBifrost]);

  const toggleKiln = (
    <div className="flex flex-row justify-between gap-5">
      <Card
        className={`w-2/3 ${kilnServiceData.success ? kilnData.state ? "bg-success" : "bg-content3" : "bg-danger"}`}>
        <CardBody className="pl-5 pr-5 text-center">
          {kilnServiceData.success ? kilnData.state ? "POWERED ON" : "POWERED OFF" : "ERROR POWERING KILN"}
        </CardBody>
      </Card>
      <Button className="w-1/3 text-h1 h-12" color="primary" onPress={toggleKilnState}>
        {kilnData.state ? "STOP KILN" : "START KILN"}
        {kilnData.state ? <Square size="15" fill="white"/> : <Power size="15"/>}
      </Button>
    </div>
  );

  const tempInput = ({children, ...props}) => (
    <output {...props}>
      <Tooltip
        className="text-tiny text-default-500 rounded-md"
        content="Press Enter to confirm"
        placement="centre"
      >
        <input
          aria-label="Temperature value"
          className="px-1 py-0.5 w-12 text-right text-small text-default-700 font-medium bg-default-100 outline-none transition-colors rounded-small border-medium border-transparent hover:border-primary focus:border-primary"
          type="text"
          value={goalTemp}
          onChange={(e) => {
            const v = e.target.value;

            setGoalTemp(v);
          }}
          onKeyDown={(e) => {
            if (e.key === "Enter" && !isNaN(Number(goalTemp))) {
              setGoalTemp(Number(goalTemp));
            }
          }}
        />
      </Tooltip>
    </output>
  )

  // <div className="flex content-stretch gap-5">
  const goalTempSlider = (


    <Slider
      size="lg"
      classNames={{
        base: "",
        size: "lg",
        label: "text-medium",
        track: "mx-0",

      }}
      color="primary"
      label="Target"
      maxValue={maxTemp}
      minValue={0}



      // eslint-disable-next-line @typescript-eslint/no-unused-vars
      renderValue={tempInput}

      // we extract the default children to render the input
      step={0.01}
      value={goalTemp}
      onChange={(v) => {
        setGoalTemp(Array.isArray(v) ? v[0] : v);
      }}
      onChangeEnd={sendTarget}
    />
  );

  const sensors = [
    {
      name: "THERMISTOR 1",
      enabled: false
    },
    {
      name: "THERMISTOR 2",
      enabled: false
    },
    {
      name: "INFRARED",
      enabled: true
    }
  ];

  return (
    <Card {...props} className="space-y-3 p-3">
      <CardHeader className="text-h1 p-0">Kiln</CardHeader>
      {toggleKiln}
      <Card className="space-y-3 p-3 bg-content2" shadow="sm">
        <SubCardLabel>TEMPERATURE</SubCardLabel>
        {goalTempSlider}
        {sensors.map((element, index) =>
          (element.enabled) &&
            <OverlayedProgress
                key={index}
                size="lg"
                value={kilnData.temp[index]}
                maxValue={maxTemp[index]}
                aria-label="Temperature Sensor Reading"
                autoColor={true}
                disableAnimation={false}
            >
                <div className="grid grid-flow-col gap-3 text-small">
                    <span>{element.name}</span>
                    <span> {kilnData.temp[index]}&deg;C</span>
                </div>
            </OverlayedProgress>
        )}
      </Card>

    </Card>
  );
};

export default KilnWidget;