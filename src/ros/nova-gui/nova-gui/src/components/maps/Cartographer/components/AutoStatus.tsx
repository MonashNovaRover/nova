import {Chip} from "@nextui-org/react";
import {useEffect} from "react";
import {useSelector} from "react-redux";
import {RootState} from "../../../../redux/RootState.ts";
import {useBifrost} from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../../ros/topics/rosTopic.ts";

interface AutoStatusProps {
}

type ChipColor = "primary" | "secondary" | "warning" | "success" | "danger";

const variants: [string, ChipColor, string][] = [
  ["Idle", "primary", "border-blue-500"],
  ["Traversing", "secondary", "border-purple-500"],
  ["Searching", "warning", "border-yellow-500"],
  ["Arrived Successfully", "success", "border-green-500"],
  ["Arrived Unsuccessfully", "danger", "border-red-500"],
];

export const AutoStatus : React.FC<AutoStatusProps> = () => {
  const bifrost = useBifrost({topic: RosTopic.AUTO_STATUS})
  const autoStatus = useSelector((state: RootState) => state.autoStatus.status)

  useEffect(() => {
    bifrost.syncWithTopic()
  }, [bifrost]);

  useEffect(() => {
    console.log(autoStatus)

  }, [autoStatus]);

  return (
    <Chip radius='md' size="lg" variant="dot" color={variants[autoStatus][1]} className={`h-10 border-2 ${variants[autoStatus][2]}`}>
      {variants[autoStatus][0]}
    </Chip>
  )
}

export default AutoStatus;
