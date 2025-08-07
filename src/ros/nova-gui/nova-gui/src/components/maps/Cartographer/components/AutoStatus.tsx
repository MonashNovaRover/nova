import {Chip} from "@nextui-org/react";
import {useEffect} from "react";
import {useSelector} from "react-redux";
import {RootState} from "../../../../redux/RootState.ts";
import {useBifrost} from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../../ros/topics/rosTopic.ts";

interface AutoStatusProps {
}

const variants: [string, "default" | "secondary" | "warning" | "success" | "danger"][] = [
  ["Idle", "default"],
  ["Traversing", "secondary"],
  ["Searching", "warning"],
  ["Arrived Successfully", "success"],
  ["Arrived Unsuccessfully", "danger"],
]

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
    <Chip radius='sm' color={variants[autoStatus][1]} size="md">
      {variants[autoStatus][0]}
    </Chip>
  )
}

export default AutoStatus;
