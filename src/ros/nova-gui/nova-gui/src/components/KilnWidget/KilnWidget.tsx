import React, {useState, useEffect, useCallback} from "react";
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

  // set up to 'refresh' kilnData state
  const kilnData = useSelector(
    (state: RootState) => state.kilnData
  );

  const kilnServiceData = useSelector(
    (state: RootState) => state.kilnCommand
  );

  const dataBifrost = useBifrost({topic: RosTopic.KILN_DATA});
  const serviceBifrost = useBifrost({service: RosService.KILN_COMMAND});
  const toggleKilnState = () => serviceBifrost.callServiceToRedux({target: Math.round(goalTemp), state: !kilnData.state});
  const sendTarget = () => {
    try {
      serviceBifrost.callServiceToRedux({target: Math.round(goalTemp), state: kilnData.state}, {noErrorToast: false, responseToast: true});
    } catch (e) {
      console.error(e);
    }
  }

  const setRoundGoalTemp = useCallback((goalTemp: number) => {
    const roundedGoalTemp = Math.round(goalTemp);
    setGoalTemp(roundedGoalTemp);
  }, [setGoalTemp]);

  const showRoundedTarget = useCallback((inputGoalTemp: string) => {
    //Altenrate method (rounds down always): const roundedInputGoalTemp = inputGoalTemp.split('.')[0];
    const roundedInputGoalTemp = Math.round(Number(inputGoalTemp));
    setInputGoalTemp(String(roundedInputGoalTemp));
  }, [setInputGoalTemp]);

  const handleChange = useCallback((goalTemp: number | number[]) => {
    if (Array.isArray(goalTemp)) {
      console.error("goalTemp was unexpectedly an array?? what?");
      return;
    }

    if (isNaN(Number(goalTemp))) return;

    setRoundGoalTemp(Number(goalTemp));
    setInputGoalTemp(goalTemp.toString());
  }, [setRoundGoalTemp, setInputGoalTemp]);

  useEffect(() => {
    dataBifrost.syncWithTopic(); // calling ros bridge to subscribe to topic
    // update max temps if current temps exceed them
    if (kilnData.temp[0] > maxTemp) {
      setMaxTemp(1.1 * kilnData.temp[0]);
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
        label: "text-medium",
      }}
      color="primary"
      label="Target"
      maxValue={maxTemp}
      minValue={0}

      //renderValue={inputGoalTemp}
      renderValue={({children, ...props}) => (

        <output {...props}>
          <Tooltip
            className="text-tiny text-default-500 rounded-md"
            content="Press Enter to confirm"
            placement="left"
          >
            <input
              aria-label="Temperature value"
              className="px-1 py-0.5 w-12 text-right text-small text-default-700 font-medium bg-default-100 outline-none transition-colors rounded-small border-medium border-transparent hover:border-primary focus:border-primary"
              type="text"
              value={inputGoalTemp}
              onChange={(e) => {
                const v = e.target.value;

                setInputGoalTemp(v);
              }}
              onKeyDown={(e) => {
                if (e.key === "Enter" && !isNaN(Number(inputGoalTemp))) {
                  setRoundGoalTemp(Number(inputGoalTemp));
                  showRoundedTarget(inputGoalTemp);
                  sendTarget();
                }
              }}
            />
          </Tooltip>
        </output>
      )}

      // we extract the default children to render the input
      step={1}
      value={goalTemp}
      onChange={(v) => {
        handleChange(v);
        setGoalTemp(Array.isArray(v) ? v[0] : v);
      }}
      onChangeEnd={sendTarget}
    />
  );

  const sensors = [
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
                maxValue={maxTemp}
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