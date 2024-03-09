import { useState, useEffect } from "react";
import { DriveProgress } from "../DriveSpeedWidget/DriveProgress";
import { Button, Card, CardHeader, CardBody, CardProps } from "@nextui-org/react";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../ros/topics/rosTopic";
import { RosService } from "../../ros/services/rosService";

interface KilnWidgetProps extends CardProps {

}


const KilnWidget: React.FC<KilnWidgetProps> = (props) => {
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

    const toggleKiln = <div className="flex flex-row justify-around">
                        <Card className={kilnData.state ? "w-1/2 bg-green-600" : "w-1/2 bg-zinc-800"}>
                            <CardBody className="text-center">
                                {kilnData.state ? "POWERED ON" : "POWERED OFF"}
                            </CardBody>
                        </Card>
                        <Button className="w-1/3 text-lg h-12" color="primary" onPress={()=>{
                                if (!kilnData.state) {
                                    setTime(0);
                                };
                                toggleKilnState();
                            }}>
                            { kilnData.state ? "STOP KILN" : "START KILN" }
                        </Button>
                    </div>

    const sensorNameList = ["ONE", "TWO", "THREE"]

    return (
        <Card {...props} className="w-[28rem] m-3 flex flex-col space-y-3 p-3 bg-zinc-900">
            <CardHeader className="p-0 text-2xl">Kiln</CardHeader>
            <Card className="flex flex-col justify-around h-28 bg-zinc-700">
                <CardHeader className={kilnServiceData.success ? "mx-3 p-0 text-xl" : "mx-3 p-0 text-xl text-rose-600"}>STATUS{kilnServiceData.success ? "" : ": NODE RESPONSE ERROR"}</CardHeader>
                {toggleKiln}
            </Card>
            <Card className="h-40 justify-between flex flex-col p-3 pb-4 bg-zinc-700">
                <CardHeader className="mb-3 p-0 text-xl">TEMPERATURE</CardHeader>
                {sensorNameList.map((element, index) => 
                    <DriveProgress
                        key={index}
                        classNames={{
                            track: "bg-zinc-500"
                        }}
                        size="lg" autoColor={true} aria-label="temp1" maxValue={maxTemp[index]} value={kilnData.temp[index]}>
                        SENSOR {element}: {kilnData.temp[index]}&deg;C
                    </DriveProgress >
                )}
            </Card>
        </Card>
    );
};

export default KilnWidget;