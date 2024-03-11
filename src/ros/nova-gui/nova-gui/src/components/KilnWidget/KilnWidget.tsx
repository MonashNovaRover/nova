import { useState, useEffect } from "react";
import { DriveProgress } from "../DriveSpeedWidget/DriveProgress";
import { Button, Card, CardHeader, CardBody, CardProps } from "@nextui-org/react";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../ros/topics/rosTopic";
import { RosService } from "../../ros/services/rosService";
import { SubCardLabel } from "../shared/Labels";
import { Square, Power } from "react-feather";

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

    const toggleKiln = <div className="flex flex-row justify-between">
                        <Card className={kilnData.state ? "bg-success" : "bg-content2"}>
                            <CardBody className="pl-5 pr-5">
                                {kilnData.state ? "POWERED ON" : "POWERED OFF"}
                            </CardBody>
                        </Card>
                        <Button className="text-h1 h-12" color="primary" onPress={()=>{
                                if (!kilnData.state) {
                                    setTime(0);
                                };
                                toggleKilnState();
                            }}>
                            { kilnData.state ? "STOP KILN" : "START KILN" }
                            { kilnData.state ? <Square size="15" fill="white" />: <Power size="15"/>}
                        </Button>
                    </div>

    const sensorNameList = ["INFRARED"]

    return (
        <Card {...props} className="space-y-3 p-3">
            <CardHeader className="text-h1 p-0">Kiln</CardHeader>
                {toggleKiln}
            <SubCardLabel>TEMPERATURE</SubCardLabel>
                {sensorNameList.map((element, index) => 
                    <DriveProgress
                        key={index}
                        size="lg"
                        value={kilnData.temp[index]}
                        maxValue={maxTemp[index]} 
                        aria-label="Temperature Sensor Reading" 
                        autoColor={true} 
                        disableAnimation={false}
                    >
                        SENSOR {element}: {kilnData.temp[index]}&deg;C
                    </DriveProgress>
                )}

        </Card>
    );
};

export default KilnWidget;