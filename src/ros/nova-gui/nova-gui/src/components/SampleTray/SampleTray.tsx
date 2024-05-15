import { useState } from "react";
import SegmentedPicker from "../SegmentedPicker/SegmentedPicker";
import { useRosAction } from "../../hooks/ros/useRosAction";
import { RosAction } from "../../ros/actions/RosAction";
import { Button } from "@nextui-org/react";

const SAMPLE_TRAY_POSITIONS = [
    {
        display: "Zero",
        value: "zero",
    },
    {
        display: "Sample One",
        value: "sample_one",
    },
    {
        display: "Sample Two",
        value: "sample_two",
    },
    {
        display: "Cache",
        value: "cache",
    },
    {
        display: "Clean",
        value: "clean",
    },
];

interface SampleTrayProps {
}

const SampleTray: React.FC<SampleTrayProps> = () => {
    
    const [sampleTrayPosition, setSampleTrayPosition] = useState<number>(0);
    const {sendGoal, feedback} = useRosAction(RosAction.SCIENCE_SAMPLE_TRAY);

    const onApply = () => {
        console.log("Applying Sample Tray Position")
        const action = {
            goal: SAMPLE_TRAY_POSITIONS[sampleTrayPosition].value,
        }
        console.log(action)
        sendGoal(action);
    }

    return (
        <>
            <SegmentedPicker 
                fullWidth
                className={"grow"}
                onIndexChange={(i) => setSampleTrayPosition(i)}
                >
            {SAMPLE_TRAY_POSITIONS.map((position, i) => (
                <div key={i} className={i === sampleTrayPosition ? "text-primary-500" : ""}>{position.display}</div>
                ))}
            </SegmentedPicker>
            <div className="text-center">{`Current Position: ${feedback?.current_position}, Goal Position: ${feedback?.goal_position}`}</div>
            <Button onClick={() => onApply()}>Apply</Button>
        </>
      );
  };
  export default SampleTray;