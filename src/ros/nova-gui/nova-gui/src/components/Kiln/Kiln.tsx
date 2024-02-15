import { useState } from "react";
import { DriveProgress } from "../DriveSpeedWidget/DriveProgress";
import { Button } from "@nextui-org/react";

// TO DO: add ROS

const Kiln: React.FC<{}> = () => {
    const temperature = 49;

    const [time, setTime] = useState(0)
    const [kilnStatus, setKilnStatus] = useState(false);

    setTimeout(() => {kilnStatus ? setTime(time + 1) : null}, 1000);
    
    return (
        <div className="w-80 m-1 flex flex-col space-y-2">
            <div className="flex flex-row justify-between">
                <Button className="w-5/12 text-lg h-6" color={ kilnStatus ? "success" : "warning" } onPress={()=>{
                    if (!kilnStatus) {setTime(0);};
                    setKilnStatus(!kilnStatus);
                }}>
                    { kilnStatus ? "On" : "Off" }
                </Button>
                <Button className="w-1/2 text-lg h-6" color="primary" onPress={()=>{
                    if (!kilnStatus) {setTime(0);};
                }}>
                    {~~(time/60)}:{time%60<10 ? "0" : null}{time%60}
                </Button>
            </div>
            <DriveProgress size="lg" autoColor={true} aria-label="Temperature" maxValue={100} value={temperature}>
                Temperature: {temperature} C
            </DriveProgress >
            <div className="text-center bg-pink-500">
                CAMERA 
            </div>
        </div> // Add Camera component instead when configured
    )
}

export default Kiln;