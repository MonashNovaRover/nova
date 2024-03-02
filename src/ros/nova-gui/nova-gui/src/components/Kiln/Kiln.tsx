import { useState, useEffect } from "react";
import { DriveProgress } from "../DriveSpeedWidget/DriveProgress";
import { Avatar, Button, Badge } from "@nextui-org/react";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../ros/topics/rosTopic";
import { RosService } from "../../ros/services/rosService";

// TO DO: add ROS for turning kiln off and on once it exists

const Kiln: React.FC = () => {
    const [time, setTime] = useState(0);
    const [maxTemp, setMaxTemp] = useState([100, 100, 100]);

    const kilnData = useSelector(
        (state: RootState) => state.kilnData
    );

    const kilnServiceData = useSelector(
        (state: RootState) => state.kilnCommand
    );

    const dataBifrost = useBifrost({ topic: RosTopic.KILN_DATA });
    const serviceBifrost = useBifrost({ service: RosService.KILN_COMMAND});
    const toggleKilnState = () => serviceBifrost.callServiceToRedux(!kilnData.state);

    useEffect(() => {
        dataBifrost.syncWithTopic();
        // update max temps if current temps exceed them
        maxTemp.forEach((element, index) => { if (kilnData.temp[index] > element) {
            let result = maxTemp;
            result[index] = kilnData.temp[index];
            setMaxTemp(result);
        }})
    }, [dataBifrost]);

    setTimeout(() => {kilnData.state ? setTime(time + 1) : null}, 1000);
    let toggleKiln = <Button className="w-1/4 text-lg h-8" color="primary" onPress={()=>{
                            if (!kilnData.state) {
                                setTime(0);
                            };
                            toggleKilnState();
                        }}>
                        { kilnData.state ? "Turn Off" : "Turn On" }
                    </Button>

    let stateError;
    if (kilnServiceData.success) {
        stateError = toggleKiln
    } else {
        stateError = <Badge content="error" color="danger" size="sm">
                        {toggleKiln}
                    </Badge>
    }

    return (
        <div className="w-80 m-1 flex flex-col space-y-2">
            <div className="flex flex-row justify-between">
                {stateError}
                <Avatar size="sm" color={kilnData.state ? "success" : "warning"} name={kilnData.state ? "On" : "Off"} />
                <Button className="w-1/2 text-lg h-8" color="primary" onPress={()=>{
                    if (!kilnData.state) {setTime(0);};
                }}>
                    {~~(time/60)}:{time%60<10 ? "0" : null}{time%60}
                </Button>
            </div>
            <DriveProgress size="lg" autoColor={true} aria-label="temp1" maxValue={maxTemp[0]} value={kilnData.temp[0]}>
                Sensor 1 Temperature: {kilnData.temp[0]} C
            </DriveProgress >
            <DriveProgress size="lg" autoColor={true} aria-label="temp2" maxValue={maxTemp[1]} value={kilnData.temp[1]}>
                Sensor 2 Temperature: {kilnData.temp[1]} C
            </DriveProgress >
            <DriveProgress size="lg" autoColor={true} aria-label="temp3" maxValue={maxTemp[2]} value={kilnData.temp[2]}>
                Sensor 3 Temperature: {kilnData.temp[2]} C
            </DriveProgress >
            <div className="text-center bg-slate-900 pt-10 pb-10">
                CAMERA HERE WHEN CONFIGURED
            </div>
        </div> // Add Camera component instead when configured
    );
};

export default Kiln;