import { useState, useEffect } from "react";
import { DriveProgress } from "../DriveSpeedWidget/DriveProgress";
import { Avatar, Button, Badge, Card, CardHeader } from "@nextui-org/react";
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
    const toggleKilnState = () => serviceBifrost.callServiceToRedux({state: !kilnData.state});

    useEffect(() => {
        dataBifrost.syncWithTopic();
        // update max temps if current temps exceed them
        maxTemp.forEach((element, index) => { if (kilnData.temp[index] > element) {
            let result = maxTemp;
            result[index] = 1.1*kilnData.temp[index];
            setMaxTemp(result);
        }})
    }, [dataBifrost]);

    setTimeout(() => {kilnData.state ? setTime(time + 1) : null}, 1000);

    let toggleKiln = <>
                        <Button className={kilnData.state ? "w-1/4 text-xl h-10" : "w-1/2 text-lg h-10"} color="primary" onPress={()=>{
                                if (!kilnData.state) {
                                    setTime(0);
                                };
                                toggleKilnState();
                            }}>
                            { kilnData.state ? "TURN OFF" : "TURN ON" }
                        </Button>
                        <Avatar className={kilnData.state ? "w-1/6 text-xl h-10" : "w-1/3 text-lg h-10"} size="sm" color={kilnData.state ? "success" : "warning"} name={kilnData.state ? "ON" : "OFF"} />
                    </>

    const stateError = () => {
        if (kilnServiceData.success) {
            return toggleKiln
        } else {
            return <Badge content="error" color="danger" size="sm">
                        {toggleKiln}
                    </Badge>
        }
    }

    const timer = () => {
        if (kilnData.state) {
            let timeInMinutes = ~~(time/60);
            let timeInMinutesString = timeInMinutes.toString();
            let timeInSecondsString;
            if (time%60 < 10) {
                let timeInSeconds = time%60
                timeInSecondsString = "0" + timeInSeconds.toString()
            } else {
                let timeInSeconds = time%60
                timeInSecondsString = timeInSeconds.toString()
            }
            return <>
                <Button className="w-1/3 text-xl h-10" color="primary" onPress={()=>{
                    if (!kilnData.state) {setTime(0);};
                }}> RESET TIMER
                </Button>
                <Avatar className="w-1/6 text-xl h-10" size="sm" color={kilnData.state ? "success" : "warning"} name={timeInMinutesString + ":" + timeInSecondsString} />
            </>;
        }
    }

    return (
        <Card className="w-[28rem] m-1 flex flex-col space-y-2 p-1">
            <CardHeader className="ml-2 p-0 text-2xl">Kiln</CardHeader>
            <div className="flex flex-row justify-between">
                {stateError()}
                {timer()}
            </div>
            <Card className="h-32 justify-around flex flex-col px-2 bg-slate-900">
                TEMPERATURE
                <DriveProgress
                    size="lg" autoColor={true} aria-label="temp1" maxValue={maxTemp[0]} value={kilnData.temp[0]}>
                    SENSOR ONE: {Math.round(kilnData.temp[0])}&deg;C
                </DriveProgress >
                <DriveProgress
                    size="lg" autoColor={true} aria-label="temp2" maxValue={maxTemp[1]} value={kilnData.temp[1]}>
                    SENSOR TWO: {Math.round(kilnData.temp[1])}&deg;C
                </DriveProgress >
                <DriveProgress
                    size="lg" autoColor={true} aria-label="temp3" maxValue={maxTemp[2]} value={kilnData.temp[2]}>
                    SENSOR THREE: {Math.round(kilnData.temp[2])}&deg;C
                </DriveProgress >
            </Card>
            <div className="text-center bg-slate-900 pt-10 pb-10">
                CAMERA HERE WHEN CONFIGURED
            </div>
        </Card> // Add Camera component instead when configured
    );
};

export default Kiln;