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
  const [maxTemp, setMaxTemp] = useState(120);
  const [goalTemp, setGoalTemp] = useState<number>(25);
  const [inputGoalTemp, setInputGoalTemp] = useState<string>("25");

  const handleChange = (goalTemp) => {
    if (isNaN(Number(goalTemp))) return;

    setGoalTemp(goalTemp);
    setInputGoalTemp(goalTemp.toString());
  };

  const kilnData = useSelector(
    (state: RootState) => state.kilnData
  );

  const kilnServiceData = useSelector(
    (state: RootState) => state.kilnCommand
  );

  const dataBifrost = useBifrost({topic: RosTopic.KILN_DATA});
  const serviceBifrost = useBifrost({service: RosService.KILN_COMMAND});
  const toggleKilnState = () => serviceBifrost.callServiceToRedux({state: !kilnData.state});

  useEffect(() => {
    dataBifrost.syncWithTopic();
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

  // <div className="flex content-stretch gap-5">
  const goalTempSlider = (

    <Slider
      size="lg"
      classNames={{
        base: "",
        size: "lg",
        label: "text-medium",
        track: "mx-0",
        filler: "bg-gradient-to-r from-primary-500 to-primary-500",
      }}
      color="foreground"
      label="Target"
      maxValue={120}
      minValue={0}

      renderThumb={(props) => (
        <div
          {...props}
          className="group p-1 top-1/2 bg-background border-small border-default-200 dark:border-default-400/50 shadow-medium rounded-full cursor-grab data-[dragging=true]:cursor-grabbing"
        >
          <span className="transition-transform bg-gradient-to-br shadow-small from-secondary-100 to-secondary-500 rounded-full w-5 h-5 block group-data-[dragging=true]:scale-80" />
        </div>
      )}

      // eslint-disable-next-line @typescript-eslint/no-unused-vars
      renderValue={({children, ...props}) => (
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
      )}
      size="sm"
      // we extract the default children to render the input
      step={0.01}
      value={goalTemp}
      onChange={handleChange}
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