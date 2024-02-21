import { useState, useEffect } from "react";
import { DriveProgress } from "../DriveSpeedWidget/DriveProgress";
import { Button, Input } from "@nextui-org/react";
import { useSelector } from "react-redux";
import { RootState } from "../../redux/RootState";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RosTopic } from "../../ros/topics/rosTopic";

// TO DO: add ROS

const Kiln: React.FC = () => {
    const [time, setTime] = useState(0)
    const [kilnStatus, setKilnStatus] = useState(false);
    const [toggleKiln, setToggleKiln] = useState(true);
    const [kilnID, setKilnID] = useState(0);
    const [maxTemp, setMaxTemp] = useState(100);
    const [temp, setTemp] = useState(0);

    let timer = setTimeout(() => {kilnStatus ? setTime(time + 1) : null}, 1000);
    let toggleTimer;

    const kilnTempStore = useSelector(
        (state: RootState) => state.kilnTempStore
    );

    const bifrost = useBifrost({ topic: RosTopic.KILN_TEMP });

    useEffect(() => {
        bifrost.syncWithTopic();
        if (kilnTempStore.id == kilnID) {
            setTemp(kilnTempStore.resistance)
            if (temp > maxTemp) {
                setMaxTemp(temp);
            }
        }
    }, [bifrost]);
    
    return (
        <div className="w-80 m-1 flex flex-col space-y-2">
            <div className="flex flex-row justify-between">
                <Input
                    placeholder="Kiln ID: [0, 3]"
                    value={kilnID.toString()}
                    labelPlacement="outside"
                    onValueChange={(value: string) => setKilnID(+value)}
                    className="h-8 w-1/3"
                    classNames={{input: ["text-center", "placeholder:text-center"]}}
                    size="sm"
                />
                <Button className="w-1/5 text-lg h-8" color={ kilnStatus ? "success" : "warning" } onPress={()=>{
                    if (toggleKiln) {
                        setToggleKiln(false);
                        toggleTimer = setTimeout(() => setToggleKiln(true), 1000);
                        if (!kilnStatus) {setTime(0);};
                        setKilnStatus(!kilnStatus);
                    }
                }}>
                    { kilnStatus ? "On" : "Off" }
                </Button>
                <Button className="w-1/4 text-lg h-8" color="primary" onPress={()=>{
                    if (!kilnStatus) {setTime(0);};
                }}>
                    {~~(time/60)}:{time%60<10 ? "0" : null}{time%60}
                </Button>
            </div>
            <DriveProgress size="lg" autoColor={true} aria-label="Temperature" maxValue={maxTemp} value={temp}>
                Temperature: {temp} C
            </DriveProgress >
            <div className="text-center bg-pink-500">
                CAMERA HERE WHEN CONFIGURED
            </div>
        </div> // Add Camera component instead when configured
    );
};

export default Kiln;